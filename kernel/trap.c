#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "printf.h"
#include "proc.h" // 引入进程

// 在 kernelvec.S 中定义的汇编入口点
extern void kernelvec();

// 用于时钟中断测试的全局变量
volatile int ticks;

// (任务5) 时钟中断处理函数
void clock_handler() {
    ticks++;

    // 每 100 个 tick 打印一次 (避免刷屏)
    // if (ticks % 100 == 0) {
    //     printf("tick %d\n", ticks);
    // }
    
    // 抢占式调度:
    // 如果当前有进程在运行, 强制它 yield
    if(myproc() != 0 && myproc()->state == RUNNING) {
        yield();
    }

    // 设置下一次时钟中断
    // 大约 0.1 秒后
    w_stimecmp(r_time() + 1000000);
}

// (任务6) 异常处理函数
void exception_handler() {
    uint64_t scause = r_scause();
    uint64_t sepc = r_sepc();
    uint64_t stval = r_stval();

    printf("--- KERNEL EXCEPTION ---\n");
    printf("scause 0x%lx (", scause);

    switch (scause) {
        case 2:  printf("Illegal instruction"); break;
        case 5:  printf("Load access fault"); break;
        case 7:  printf("Store/AMO access fault"); break;
        case 12: printf("Instruction page fault"); break;
        case 13: printf("Load page fault"); break;
        case 15: printf("Store/AMO page fault"); break;
        default: printf("Unknown exception"); break;
    }
    printf(")\n");
    printf("sepc=0x%lx stval=0x%lx\n", sepc, stval);
    
    panic("Kernel exception");
}

// S-Mode 陷阱总处理程序 (由 kernelvec.S 调用)
void kerneltrap(void) {
    uint64_t scause = r_scause();
    uint64_t sstatus = r_sstatus();
    uint64_t sepc = r_sepc();

    // 检查是否从 S-Mode 陷入
    if ((sstatus & SSTATUS_SPP) == 0) {
        panic("kerneltrap: not from S-Mode");
    }

    // 检查中断是否关闭
    // (注意: 硬件在进入陷阱时会自动关闭 SIE)
    // if (r_sstatus() & SSTATUS_SIE) {
    //    panic("kerneltrap: interrupts enabled");
    // }

    // 判断是中断还是异常
    // scause 最高位为1: 中断
    if ((scause & (1UL << 63)) != 0) {
        // (任务5)
        // 判断是否为 S-Mode 时钟中断 (scause = 0x8000...05)
        if (scause == 0x8000000000000005L) {
            clock_handler();
        } 
        // 判断是否为 S-Mode 外部中断 (UART等) (scause = 0x8000...09)
        else if (scause == 0x8000000000000009L) {
            // (我们将在实验5中实现PLIC)
            printf("External interrupt (PLIC) received.\n");
        }
        else {
            printf("unexpected interrupt scause 0x%lx\n", scause);
            panic("kerneltrap interrupt");
        }
    } 
    // scause 最高位为0: 异常
    else {
        // (任务6)
        exception_handler();
    }

    // 恢复 sepc (因为 exception_handler 可能会修改它，虽然目前不会)
    w_sepc(sepc);
    w_sstatus(sstatus);
}

// (任务3) 初始化中断系统
void trap_init(void) {
    // 初始化时钟计数器
    ticks = 0;
}

// (任务3) 在 S-Mode 下设置中断向量表
void trapinithart(void) {
    // 将 stvec 设置为 S-Mode 陷阱入口 (kernelvec.S)
    //如果发生任何中断或异常（统称 Trap），立即跳转到 kernelvec 这个地址。
    w_stvec((uint64_t)kernelvec);
    printf("trapinithart: stvec set to 0x%lx\n", (uint64_t)kernelvec);
}