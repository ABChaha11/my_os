#ifndef __RISCV_H__
#define __RISCV_H__

#include "types.h"

// --- CSR (Control and Status Registers) 操作 ---

// S-Mode 状态寄存器 (sstatus)
static inline uint64_t r_sstatus() {
    uint64_t x;
    asm volatile("csrr %0, sstatus" : "=r"(x));
    return x;
}
static inline void w_sstatus(uint64_t x) {
    asm volatile("csrw sstatus, %0" : : "r"(x));
}
#define SSTATUS_MPP_MASK (3L << 11) // M-Mode: 3, S-Mode: 1, U-Mode: 0
#define SSTATUS_MPP_S    (1L << 11)
#define SSTATUS_SPP      (1L << 8)  // 陷阱发生前的特权级 (0=U, 1=S)
#define SSTATUS_SPIE     (1L << 5)  // 陷阱发生前 sstatus.SIE 的值
#define SSTATUS_SIE      (1L << 1)  // S-Mode 中断使能

// S-Mode 中断使能寄存器 (sie)
static inline uint64_t r_sie() {
    uint64_t x;
    asm volatile("csrr %0, sie" : "=r"(x));
    return x;
}
static inline void w_sie(uint64_t x) {
    asm volatile("csrw sie, %0" : : "r"(x));
}
#define SIE_SEIE (1L << 9) // S-Mode 外部中断使能
#define SIE_STIE (1L << 5) // S-Mode 时钟中断使能
#define SIE_SSIE (1L << 1) // S-Mode 软件中断使能

// S-Mode 陷阱向量基地址 (stvec)
static inline uint64_t r_stvec() {
    uint64_t x;
    asm volatile("csrr %0, stvec" : "=r"(x));
    return x;
}
static inline void w_stvec(uint64_t x) {
    asm volatile("csrw stvec, %0" : : "r"(x));
}

// S-Mode 异常程序计数器 (sepc)
static inline uint64_t r_sepc() {
    uint64_t x;
    asm volatile("csrr %0, sepc" : "=r"(x));
    return x;
}
static inline void w_sepc(uint64_t x) {
    asm volatile("csrw sepc, %0" : : "r"(x));
}

// S-Mode 陷阱原因 (scause)
static inline uint64_t r_scause() {
    uint64_t x;
    asm volatile("csrr %0, scause" : "=r"(x));
    return x;
}

// S-Mode 陷阱值 (stval)
static inline uint64_t r_stval() {
    uint64_t x;
    asm volatile("csrr %0, stval" : "=r"(x));
    return x;
}

// S-Mode 地址翻译和保护 (satp)
static inline uint64_t r_satp() {
    uint64_t x;
    asm volatile("csrr %0, satp" : "=r"(x));
    return x;
}
static inline void w_satp(uint64_t x) {
    asm volatile("csrw satp, %0" : : "r"(x));
}

// 刷新 TLB
static inline void sfence_vma() {
    asm volatile("sfence.vma zero, zero");
}


// --- M-Mode CSRs (用于 start.c) ---

// M-Mode 状态寄存器 (mstatus)
static inline uint64_t r_mstatus() {
    uint64_t x;
    asm volatile("csrr %0, mstatus" : "=r"(x));
    return x;
}
static inline void w_mstatus(uint64_t x) {
    asm volatile("csrw mstatus, %0" : : "r"(x));
}

#define MSTATUS_MPP_MASK (3L << 11)
#define MSTATUS_MPP_S    (1L << 11)

// M-Mode 异常程序计数器 (mepc)
static inline void w_mepc(uint64_t x) {
    asm volatile("csrw mepc, %0" : : "r"(x));
}

// M-Mode 委托寄存器
static inline void w_medeleg(uint64_t x) {
    asm volatile("csrw medeleg, %0" : : "r"(x));
}
static inline void w_mideleg(uint64_t x) {
    asm volatile("csrw mideleg, %0" : : "r"(x));
}

// M-Mode 中断使能 (mie)
static inline uint64_t r_mie() {
    uint64_t x;
    asm volatile("csrr %0, mie" : "=r"(x));
    return x;
}
static inline void w_mie(uint64_t x) {
    asm volatile("csrw mie, %0" : : "r"(x));
}
#define MIE_MEIE (1L << 11) // M-Mode 外部中断使能
#define MIE_MTIE (1L << 7)  // M-Mode 时钟中断使能
#define MIE_MSIE (1L << 3)  // M-Mode 软件中断使能
#define MIE_SEIE (1L << 9) // S-Mode 外部中断使能 (在 mie 中)
#define MIE_STIE (1L << 5) // S-Mode 时钟中断使能 (在 mie 中)
#define MIE_SSIE (1L << 1) // S-Mode 软件中断使能 (在 mie 中)


// M-Mode 物理内存保护 (PMP)
static inline void w_pmpaddr0(uint64_t x) {
    asm volatile("csrw pmpaddr0, %0" : : "r"(x));
}
static inline void w_pmpcfg0(uint64_t x) {
    asm volatile("csrw pmpcfg0, %0" : : "r"(x));
}

// M-Mode Hart ID
static inline uint64_t r_mhartid() {
    uint64_t x;
    asm volatile("csrr %0, mhartid" : "=r"(x));
    return x;
}

// S-Mode Thread Pointer (tp)
static inline void w_tp(uint64_t x) {
    asm volatile("csrw tp, %0" : : "r"(x));
}

// M-Mode 计数器使能
static inline uint64_t r_mcounteren() {
    uint64_t x;
    asm volatile("csrr %0, mcounteren" : "=r"(x));
    return x;
}
static inline void w_mcounteren(uint64_t x) {
    asm volatile("csrw mcounteren, %0" : : "r"(x));
}

// M-Mode Sstc 扩展使能
static inline uint64_t r_menvcfg() {
    uint64_t x;
    asm volatile("csrr %0, menvcfg" : "=r"(x));
    return x;
}
static inline void w_menvcfg(uint64_t x) {
    asm volatile("csrw menvcfg, %0" : : "r"(x));
}
#define MENVCFG_STCE (1L << 63) // S-Mode Timer Counter Enable

// M-Mode 时钟
static inline uint64_t r_time() {
    uint64_t x;
    // 使用 rdtime 伪指令
    asm volatile("csrr %0, time" : "=r"(x));
    return x;
}

// M-Mode 时钟比较器 (S-Mode 通过 w_stimecmp 访问)
static inline void w_stimecmp(uint64_t x) {
    // SBI 规范要求 M-Mode 设置 mtimecmp
    // QEMU virt 机器将 0x2004000 映射到 mtimecmp
    // 但 RISC-V 规范允许 S-Mode 写 stimecmp
    asm volatile("csrw stimecmp, %0" : : "r"(x));
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