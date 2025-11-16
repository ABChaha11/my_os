// include/spinlock.h

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "types.h"

// 互斥自旋锁
struct spinlock {
  uint64_t locked;       // 0表示未锁, 1表示锁定
  
  // 用于调试
  char *name;        // 锁的名称
  struct cpu *cpu;   // 持有锁的CPU
};

void initlock(struct spinlock *lk, char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
int holding(struct spinlock *lk);
void push_off(void);
void pop_off(void);

#endif