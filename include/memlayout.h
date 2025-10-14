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

#endif