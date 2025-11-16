#ifndef __DEFS_H__
#define __DEFS_H__

#include "types.h" // <--- 关键修复: 包含此文件以定义 pte_t
#include "proc.h" 

// 内核全局函数声明

// from main.c
void panic(char *s);
void* memset(void *dst, int c, uint64_t n); // <--- 新增
int safestrcpy(char *s, const char *t, int n); // <--- 新增

// from trap.c
void trap_init(void);
void trapinithart(void);

// from vm.c
void kvminit(void);
void kvminithart(void);
pte_t* walk(pagetable_t pt, uint64_t va, int alloc);
int kvmmap(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t size, int perm);
void kvmunmap(pagetable_t pt, uint64_t va, uint64_t size, int do_free);

// from pmm.c
void pmm_init(void);
void* alloc_page(void);
void free_page(void* pa);

// from spinlock.c
void initlock(struct spinlock *lk, char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
int holding(struct spinlock *lk);
void push_off(void);
void pop_off(void);

// from proc.c
void proc_init(void);
void scheduler(void) __attribute__((noreturn)); // <--- 在声明处使用
void sched(void);
void yield(void);
void sleep(void *chan, struct spinlock *lk);
void wakeup(void *chan);
struct proc* myproc(void);
struct cpu* mycpu(void);
void user_init(void);
int kfork(void (*entry)(void));
void kexit(int status);
int kwait(int *status);

// from swtch.S
void swtch(struct context *old, struct context *new);

#endif