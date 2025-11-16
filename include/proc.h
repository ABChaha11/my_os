// include/proc.h

#ifndef __PROC_H__
#define __PROC_H__

#include "types.h"
#include "spinlock.h"
#include "param.h"
#include "memlayout.h"

// 调度上下文
// 由 swtch.S 保存和恢复
struct context {
  uint64_t ra;  // 返回地址
  uint64_t sp;  // 栈指针

  // Callee-saved 寄存器
  uint64_t s0;
  uint64_t s1;
  uint64_t s2;
  uint64_t s3;
  uint64_t s4;
  uint64_t s5;
  uint64_t s6;
  uint64_t s7;
  uint64_t s8;
  uint64_t s9;
  uint64_t s10;
  uint64_t s11;
};

// 进程状态
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 进程控制块 (PCB)
struct proc {
  // --- 必须受 lock 保护的字段 ---
  struct spinlock lock;     // 保护此结构体
  enum procstate state;     // 进程状态
  void *chan;               // 睡眠通道 (如果 state == SLEEPING)
  int killed;               // 如果非零，进程应退出
  
  // --- 基本信息 ---
  int pid;                  // 进程ID
  int xstate;               // 退出状态 (供父进程wait读取)
  
  // --- 进程关系 ---
  struct proc *parent;      // 父进程

  // --- 内存管理 (内核线程) ---
  uint64_t kstack;            // 此进程的内核栈的虚拟地址
  
  // --- 陷阱与上下文 ---
  struct context context;      // 内核态上下文 (用于 swtch)
  
  // --- 调试与扩展 ---
  char name[16];            // 进程名 (用于调试)
};

// Per-CPU 状态
struct cpu {
  struct proc *proc;          // 此CPU上正在运行的进程 (或 null)
  struct context context;      // 调度器上下文
  int noff;                   // 关中断的嵌套深度
  int intena;                 // 关中断前，中断是否开启
};

extern struct cpu cpus[NCPU];

#endif