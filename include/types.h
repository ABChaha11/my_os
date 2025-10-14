#ifndef __TYPES_H__
#define __TYPES_H__

// 定义无符号64位整数类型，用于地址和大小计算
typedef unsigned long uint64_t;
typedef uint64_t pte_t;          // 页表项类型
typedef uint64_t* pagetable_t;   // 页表类型，本质上是一个指向PTE数组的指针

#endif