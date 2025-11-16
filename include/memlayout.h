#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

#include "types.h"

// QEMU virt 机器默认提供 128MB 物理内存.
// 物理内存的基地址是 0x80000000.
#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + 128 * 1024 * 1024)

// 页面大小 (4KB)
#define PGSIZE 4096

// 将地址向上舍入到最近的页面边界
#define PGROUNDUP(addr) (((addr) + PGSIZE - 1) & ~(PGSIZE - 1))
// 将地址向下舍入到最近的页面边界
#define PGROUNDDOWN(addr) ((addr) & ~(PGSIZE - 1))

// --- 进程相关内存布局 ---
// 每个进程的内核栈大小 (8KB)
#define KSTACKSIZE (PGSIZE * 2)
// 计算第n个进程的内核栈顶VA
// 我们将内核栈映射到内核地址空间的高位，从 PHYSTOP 向下生长
// [ KSTACK(0) ] [ G(0) ] [ KSTACK(1) ] [ G(1) ] ... PHYSTOP
// KSTACK(0) 位于 PHYSTOP - KSTACKSIZE
//#define KSTACK(i) (PHYSTOP - (i+1) * KSTACKSIZE)
// 我们为页表保留顶部的 12KB (L2 + L1 + L0) 
// 所有的内核栈都从这 12KB 之下开始分配
#define KSTACK_TOP (PHYSTOP - PGSIZE*3)
#define KSTACK(i) (KSTACK_TOP - (i+1) * KSTACKSIZE)


#endif