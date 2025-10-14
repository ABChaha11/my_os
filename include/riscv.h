#ifndef __RISCV_H__
#define __RISCV_H__

#include "types.h"

// --- CSR (Control and Status Registers) 操作 ---

// 读 satp (Supervisor Address Translation and Protection) 寄存器
static inline uint64_t r_satp() {
    uint64_t x;
    asm volatile("csrr %0, satp" : "=r"(x));
    return x;
}

// 写 satp 寄存器
static inline void w_satp(uint64_t x) {
    asm volatile("csrw satp, %0" : : "r"(x));
}

// 刷新 TLB (Translation Lookaside Buffer)
static inline void sfence_vma() {
    // 汇编指令 `sfence.vma zero, zero` 会刷新所有非全局的TLB条目
    asm volatile("sfence.vma zero, zero");
}


// --- Sv39 分页机制相关定义 ---

// 页表项 (PTE) 标志位
#define PTE_V (1L << 0) // Valid: 有效位
#define PTE_R (1L << 1) // Read: 可读
#define PTE_W (1L << 2) // Write: 可写
#define PTE_X (1L << 3) // Execute: 可执行
#define PTE_U (1L << 4) // User: 用户态可访问

// 将物理地址转换为PTE中的PPN (Physical Page Number)
// 物理地址右移12位，再左移10位
#define PA2PTE(pa) (((uint64_t)(pa) >> 12) << 10)

// 将PTE中的PPN转换为物理地址
// PTE右移10位，再左移12位
#define PTE2PA(pte) (((pte) >> 10) << 12)

// 从PTE中提取标志位
#define PTE_FLAGS(pte) ((pte) & 0x3FF)

// 从虚拟地址中提取各级页表的索引 (VPN)
#define VPN_SHIFT(level) (12 + 9 * (level))
#define VPN_MASK(va, level) ((((uint64_t)va) >> VPN_SHIFT(level)) & 0x1FF)

// SATP 寄存器格式化宏
// Sv39模式要求MODE字段为8
#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64_t)pagetable) >> 12))

#endif