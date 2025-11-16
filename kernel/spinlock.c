// kernel/spinlock.c
// 互斥自旋锁

#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

void initlock(struct spinlock *lk, char *name) {
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// 关中断
void push_off(void) {
  int old = intr_get();
  intr_off();
  if(mycpu()->noff == 0)
    mycpu()->intena = old;
  mycpu()->noff++;
}

// 开中断
void pop_off(void) {
  struct cpu *c = mycpu();
  if(intr_get())
    panic("pop_off: interruptible");
  c->noff--;
  if(c->noff < 0)
    panic("pop_off: noff < 0");
  if(c->noff == 0 && c->intena)
    intr_on();
}

// 获取锁
// 必须在循环中尝试，直到成功
void acquire(struct spinlock *lk) {
  push_off(); // 关中断
  if(holding(lk))
    panic("acquire: already holding lock");

  // 使用 RISC-V 的 'amst' (Atomic Memory Swap) 指令
  // 尝试将 lk->locked 从 0 交换为 1
  while(__sync_lock_test_and_set(&lk->locked, 1) != 0);

  // 内存屏障，确保所有后续的读写操作
  // 都在获取锁之后执行
  __sync_synchronize();

  // 记录持有锁的CPU
  lk->cpu = mycpu();
}

// 释放锁
void release(struct spinlock *lk) {
  if(!holding(lk))
    panic("release: not holding lock");

  lk->cpu = 0;

  // 内存屏障，确保所有在释放锁之前的写操作
  // 都对其他CPU可见
  __sync_synchronize();

  // 释放锁 (原子地将 locked 设置为 0)
  __sync_lock_release(&lk->locked);

  pop_off(); // 恢复中断
}

// 检查当前CPU是否持有该锁
int holding(struct spinlock *lk) {
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}