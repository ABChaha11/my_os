// kernel/proc.c

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "printf.h"
#include "tests.h"

struct cpu cpus[NCPU];
struct proc proc[NPROC];

struct proc *initproc; // 第一个进程

int nextpid = 1;
struct spinlock pid_lock;
struct spinlock wait_lock;

extern pagetable_t kernel_pagetable;
extern volatile int ticks; // <--- 声明 ticks

// 声明第一个内核线程的入口
void init_main(void);

// 声明上下文切换函数
extern void swtch(struct context*, struct context*);

// 内核线程包装函数
void kthread_wrapper(void);

// 静态函数原型
static void reparent(struct proc *p); 

// --- 辅助函数 ---

// 返回当前 CPU ID (hartid)
int cpuid() {
  return r_tp();
}

// 返回当前 CPU 结构体
struct cpu* mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// 返回当前 CPU 上正在运行的进程
struct proc* myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}


// --- 进程初始化 ---

// 初始化进程表和CPU
void proc_init(void) {
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = KSTACK(p - proc); // 计算内核栈VA
  }
  printf("proc_init: process table initialized.\n");

  // 映射所有进程的内核栈
  // (在 kvminit 之后, kvminithart 之前调用)
  for(int i=0; i < NPROC; i++) {
    // --- 关键修复: KSTACKSIZE 是 8KB (2页), 必须映射两页 ---
    char *pa_top = alloc_page(); // 分配栈顶页
    char *pa_bot = alloc_page(); // 分配栈底页
    if(pa_top == 0 || pa_bot == 0)
      panic("proc_init: kstack alloc failed");
    
    uint64_t va_top_page = KSTACK(i) - PGSIZE;
    uint64_t va_bot_page = KSTACK(i) - KSTACKSIZE; // KSTACKSIZE = 2 * PGSIZE
    
    // kvminit 已经恒等映射了这片VA。我们必须先解除映射，
    // 才能将其重新映射到我们新分配的物理页面。
    kvmunmap(kernel_pagetable, va_top_page, PGSIZE, 0); // 0 = do_not_free (pa)
    kvmunmap(kernel_pagetable, va_bot_page, PGSIZE, 0);

    kvmmap(kernel_pagetable, va_top_page, (uint64_t)pa_top, PGSIZE, PTE_R | PTE_W);
    kvmmap(kernel_pagetable, va_bot_page, (uint64_t)pa_bot, PGSIZE, PTE_R | PTE_W);
    // ----------------------------------------------------
  }
  printf("proc_init: kernel stacks mapped.\n");
}


// --- 进程分配与释放 ---

// 分配一个唯一的 PID
static int alloc_pid() {
  int pid;
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);
  return pid;
}

// 释放一个进程结构 (p->lock 必须被持有)
static void free_process(struct proc *p) {
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  // 内核栈的物理页在 kwait 中被释放
}

// 分配一个进程结构体 (PCB)
// 如果成功, 返回时持有 p->lock
static struct proc* alloc_process(void) {
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0; // 没有空闲进程

found:
  p->pid = alloc_pid();
  p->state = USED;

  // 设置内核上下文
  memset(&p->context, 0, sizeof(p->context));
  p->context.sp = p->kstack;

  return p;
}


// --- 进程生命周期 ---

// 所有新内核线程都从这里开始
// 1. 释放调度器持有的锁
// 2. 调用真正的入口函数 (存在栈上)
// 3. 如果入口函数返回, 则自动退出
void kthread_wrapper(void) {
    struct proc *p = myproc();
    
    // 1. 从栈上获取真正的入口函数地址
    // (这个地址是 kfork 在栈顶-8的位置设置的)
    uint64_t entry_func_ptr = *(uint64_t*)(p->kstack - 8);
    void (*entry)(void) = (void (*)(void))entry_func_ptr;

    // 2. 释放调度器在切换前持有的 p->lock
    //    现在可以安全地开启中断了 (如果 entry 需要)
    release(&p->lock);

    // 3. 调用真正的入口函数
    entry();
    
    // 4. 如果函数返回 (不应该发生, 除非是测试线程)
    kexit(0);
}

// 创建一个新进程 (内核线程)
// entry: 线程的入口函数
int kfork(void (*entry)(void)) {
  struct proc *p;
  struct proc *np; // new process

  p = myproc();

  // 1. 分配进程
  if((np = alloc_process()) == 0){
    return -1; // 创建失败
  }

  // 2. 设置父子关系
  np->parent = p;

  // 3. 设置上下文和栈 (同 user_init)
  //    将真正的入口函数地址压入新栈的顶部
  np->context.sp = np->kstack;
  np->context.sp -= 8;
  *(uint64_t*)(np->context.sp) = (uint64_t)entry;
  
  //    将返回地址(RA)设置为包装函数
  np->context.ra = (uint64_t)kthread_wrapper;

  // 4. 设置名称
  safestrcpy(np->name, "kthread", sizeof(np->name));

  // 5. 标记为可运行
  np->state = RUNNABLE;
  release(&np->lock); // 释放新进程的锁

  return np->pid;
}

// 进程退出
void kexit(int status) {
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // (关闭文件等... 暂时跳过)

  acquire(&wait_lock);

  // 将所有子进程过继给 initproc
  reparent(p);

  // 唤醒父进程 (如果它在 wait)
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // 永久切换到调度器
  sched();
  panic("zombie exit"); // sched不应该返回
}

// 等待一个子进程退出
// status: (可选) 用于接收子进程的退出状态
int kwait(int *status) {
  struct proc *p = myproc();
  struct proc *child;
  int havekids, pid;

  acquire(&wait_lock);

  for(;;){ // 无限循环
    havekids = 0;
    // 扫描进程表
    for(child = proc; child < &proc[NPROC]; child++){
      if(child->parent == p){
        havekids = 1;
        acquire(&child->lock);
        if(child->state == ZOMBIE){
          // 找到一个ZOMBIE子进程
          pid = child->pid;
          if(status != 0)
            *status = child->xstate; // 复制退出状态
          
          // --- 关键修复: 释放 8KB (2页) 内核栈 ---
          uint64_t kstack_base = child->kstack - KSTACKSIZE;
          kvmunmap(kernel_pagetable, kstack_base, KSTACKSIZE, 1); // 1 = do_free
          // ----------------------------------------

          free_process(child);
          release(&child->lock);
          release(&wait_lock);
          return pid;
        }
        release(&child->lock);
      }
    }

    // 如果没有子进程, 或被杀, 立即返回-1
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }
    
    // 等待子进程退出
    sleep(p, &wait_lock);
  }
}

// 将 p 的所有子进程过继给 initproc
// 调用者必须持有 wait_lock
static void reparent(struct proc *p) { 
  struct proc *pp;
  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      acquire(&pp->lock);
      pp->parent = initproc;
      release(&pp->lock);
      wakeup(initproc);
    }
  }
}

// --- 调度器 ---

void scheduler(void) { 
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){ // 无限循环
    // 开启中断, 以便时钟中断可以发生
    intr_on();

    // 轮转调度 (Round-Robin)
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // 找到了! 切换到该进程
        p->state = RUNNING;
        c->proc = p;
        
        // 切换上下文
        swtch(&c->context, &p->context);

        // 切换回来后...
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}

// 切换到调度器
void sched(void) {
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched: not holding p->lock");
  if(mycpu()->noff != 1)
    panic("sched: locks");
  if(p->state == RUNNING)
    panic("sched: RUNNING");
  if(intr_get())
    panic("sched: interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context); 
  mycpu()->intena = intena;
}

// 主动放弃CPU
void yield(void) {
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}


// --- 同步 ---

// 睡眠 (原子地释放锁, 睡眠, 醒来后重新获取锁)
void sleep(void *chan, struct spinlock *lk) {
  struct proc *p = myproc();

  if(lk == 0)
    panic("sleep: no lock");

  acquire(&p->lock); // 获取进程锁
  release(lk);       // 释放外部锁 (原子性保证)

  // 设置睡眠状态
  p->chan = chan;
  p->state = SLEEPING;

  // 切换到调度器
  sched();

  // 醒来后...
  p->chan = 0;

  // 释放进程锁, 重新获取外部锁
  release(&p->lock);
  acquire(lk);
}

// 唤醒所有在 chan 上睡眠的进程
void wakeup(void *chan) {
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){ 
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}


// --- 第一个进程 ---

// 创建第一个内核进程
void user_init(void) {
  struct proc *p;
  p = alloc_process();
  
  initproc = p;
  
  // 设置包装器和栈
  p->context.sp = p->kstack;
  p->context.sp -= 8;
  *(uint64_t*)(p->context.sp) = (uint64_t)init_main;
  p->context.ra = (uint64_t)kthread_wrapper;
  
  safestrcpy(p->name, "init", sizeof(p->name));
  p->state = RUNNABLE;
  
  release(&p->lock);
  printf("user_init: init process created.\n");
}

// 第一个进程 (initproc) 的主函数
void init_main(void) {
  printf("init_main: starting... (ticks = %d)\n", ticks);

  // 运行所有测试
  test_printf_basic();
  printf("\n");
  test_printf_edge_cases();
  printf("\n");
  
  test_pmm();
  test_vm();
  printf("\n");

  test_timer_interrupt();

  printf("\n--- All tests passed! ---\n");
  printf("Kernel is now multitasking.\n");

  // (注释掉异常测试, 否则会 panic)
  // test_exception_handling();
  
  // init 进程的工作是循环 wait
  // 它会回收所有孤儿进程
  int status;
  for(;;){
      // printf("init: waiting...\n");
      int pid = kwait(&status);
      if(pid != -1){
          printf("init: child %d exited with status %d\n", pid, status);
      } else {
          // 如果没有子进程, init 进程可以休息
          yield(); // 主动让出CPU
      }
  }
}