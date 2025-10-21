#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

// kmain.c 中的 S-Mode 内核入口
void kmain(void);

// M-Mode 时钟中断初始化
void timerinit();

/**
 * @brief M-Mode C 语言入口函数
 * (由 entry.S 调用)
 */
void start(void) {
    // --- 1. 设置 M-Mode 状态 ---
    // 设置 MSTATUS.MPP 为 S-Mode (MPP=1)
    // 这样 mret 后会进入 S-Mode
    unsigned long x = r_mstatus();
    x &= ~MSTATUS_MPP_MASK; // 清除 MPP 位
    x |= MSTATUS_MPP_S;     // 设置 MPP 为 S-Mode
    w_mstatus(x);

    // --- 2. 设置 S-Mode 入口 ---
    // 将 mret 的返回地址 (MEPC) 设置为 kmain
    w_mepc((uint64_t)kmain);

    // --- 3. 禁用 S-Mode 分页 ---
    // mret 时，S-Mode 处于裸机地址模式
    // kmain 稍后会自己开启分页
    w_satp(0);

    // --- 4. 委托中断和异常 ---
    // 委托所有同步异常 (medeleg)
    w_medeleg(0xffff);
    // 委托所有中断 (mideleg)
    w_mideleg(0xffff);

    // --- 5. 配置 S-Mode 中断 ---
    // 开启 S-Mode 的时钟中断(STIE)、外部中断(SEIE)
    w_sie(r_sie() | SIE_STIE | SIE_SEIE);
    // M-Mode 下开启对应的中断使能 (MIE)
    w_mie(r_mie() | MIE_STIE | MIE_SEIE);

    // --- 6. 配置 PMP (物理内存保护) ---
    // 允许 S-Mode 访问所有物理内存
    w_pmpaddr0(0x3fffffffffffffull); // 54位地址空间
    w_pmpcfg0(0xf); // R, W, X, TOR 模式

    // --- 7. 初始化时钟 ---
    timerinit();

    // --- 8. 切换到 S-Mode ---
    // 使用 mret 指令切换到 S-Mode，
    // CPU 将会：
    // 1. 进入 S-Mode (MPP=1)
    // 2. 跳转到 mepc (kmain)
    // 3. MIE -> SIE, mstatus.MIE -> sstatus.SIE
    asm volatile("mret");
}

/**
 * @brief M-Mode 下初始化时钟中断
 */
void timerinit() {
    // 授权 S-Mode 访问 time 和 stimecmp 寄存器
    // (设置 menvcfg.STCE 位)
    w_menvcfg(r_menvcfg() | MENVCFG_STCE);
    
    // 允许 S-Mode 读写 stimecmp 寄存器
    // (设置 mcounteren 的 CY(0), TM(1), IR(2) 位)
    // 我们只需要 TM(bit 1)
    w_mcounteren(r_mcounteren() | (1 << 1));

    // 设置下一次时钟中断
    // QEMU 中时钟频率约为 10MHz (10,000,000)
    // 1,000,000 周期大约是 0.1 秒
    uint64_t interval = 1000000;
    uint64_t now = r_time();
    w_stimecmp(now + interval);
}