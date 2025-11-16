#include "uart.h"
#include "console.h"
#include "printf.h"
#include "pmm.h"
#include "vm.h"
#include "riscv.h"
#include "memlayout.h"
#include "defs.h"
#include "trap.h"

// 声明由链接脚本提供的外部符号
extern void *_bss_start;
extern void *_bss_end;
extern void *_end;

// 声明全局时钟计数器 (来自 trap.c)
extern volatile int ticks;

/**
 * @brief 内核致命错误处理函数
 * @param s 错误信息字符串
 */
void panic(char *s){
    // 关闭中断
    intr_off();
    printf("PANIC: %s\n", s);
    // 锁定系统，进入无限循环
    while (1) {}
}

// --- 测试函数声明 ---
// (这些现在在 proc.c 的 init_main 中)
void test_printf_basic();
void test_printf_edge_cases();
void test_pmm();
void test_vm();
void test_timer_interrupt();
void test_exception_handling();


/**
 * @brief 内核的C语言入口函数
 * (现在在 S-Mode 下运行)
 */
void kmain(void) {
    // 1. 初始化串口
    uart_init();
    printf("--- riscv-os Kernel Booting ---\n");

    // 2. 初始化物理内存管理器
    pmm_init();
    
    // 3. 初始化陷阱 (中断) 系统
    trap_init();      // 初始化 ticks 等
    
    // 4. 创建内核页表
    kvminit();
    
    // 5. 初始化进程系统 (会映射内核栈)
    proc_init();

    // 6. 激活虚拟内存 (启用分页)
    kvminithart();

    // 7. 设置 S-Mode 陷阱向量
    trapinithart();   // 设置 stvec (指向 kernelvec)

    // 8. 创建第一个进程 (init)
    user_init();

    // 9. 开启 S-Mode 全局中断 (sstatus.SIE = 1)
    //    现在 stvec 已经设置好，可以安全地开启中断了
    intr_on();
    printf("kmain: S-Mode interrupts enabled.\n");

    printf("kmain: starting scheduler...\n");
    
    // 10. 启动调度器
    //     scheduler() 将永不返回
    scheduler();

    // 内核不应该执行到这里
    panic("kmain: scheduler returned");
    while (1) {}
}


// --- 完整的测试用例实现 ---
// (这些函数现在由 init_main 在第一个进程中调用)
/**
 * @brief 基础功能测试
 */
void test_printf_basic() {
    printf("--- Testing Basic Cases ---\n");
    printf("Testing integer: %d\n", 42);
    printf("Testing negative: %d\n", -123);
    printf("Testing zero: %d\n", 0);
    printf("Testing hex: 0x%x\n", 0xABC);
    printf("Testing pointer: %p\n", (void*)0x80000000);
    printf("Testing string: %s\n", "Hello World!");
    printf("Testing char: %c\n", 'X');
    printf("Testing percent: %%\n");
    printf("Multiple args: %s %d, 0x%x\n", "Number", 123, 123);
}

/**
 * @brief 边界情况测试
 */
void test_printf_edge_cases() {
    printf("--- Testing Edge Cases ---\n");
    // 2147483647 is INT_MAX
    printf("INT_MAX: %d\n", 2147483647); 
    // -2147483648 is INT_MIN
    printf("INT_MIN: %d\n", -2147483648);
    // 0xFFFFFFFF is UINT_MAX
    printf("UINT_MAX (hex): 0x%x\n", 0xFFFFFFFF);
    printf("NULL string: %s\n", (char*)0);
    printf("Empty string: %s\n", "");
}

/**
 * @brief 测试物理内存管理器 (PMM)
 */
void test_pmm() {
    printf("--- Testing PMM ---\n");
    void *p1 = alloc_page();
    printf("  alloc_page() -> 0x%lx\n", (uint64_t)p1);
    void *p2 = alloc_page();
    printf("  alloc_page() -> 0x%lx\n", (uint64_t)p2);

    if(p1 == 0 || p2 == 0) panic("alloc_page returned NULL");
    if(p1 == p2) panic("alloc_page returned same address twice");
    if((uint64_t)p1 % PGSIZE != 0 || (uint64_t)p2 % PGSIZE != 0) panic("alloc_page address not page-aligned");

    *(uint64_t*)p1 = 0x12345678ABCDEF01;
    if(*(uint64_t*)p1 != 0x12345678ABCDEF01) panic("PMM data write/read failed");
    
    free_page(p1);
    printf("  free_page(0x%lx)\n", (uint64_t)p1);
    void *p3 = alloc_page();
    printf("  alloc_page() -> 0x%lx\n", (uint64_t)p3);

    if(p3 != p1) panic("PMM free/re-alloc failed");

    free_page(p2);
    free_page(p3);
    printf("  PMM test passed.\n");
}

/**
 * @brief 测试虚拟内存 (页表)
 */
void test_vm() {
    extern pagetable_t kernel_pagetable;
    extern char etext[];

    printf("--- Testing VM ---\n");

    pte_t *text_pte = walk(kernel_pagetable, KERNBASE, 0);
    if(text_pte == 0 || (*text_pte & PTE_V) == 0) panic("kernel text not mapped");
    if((*text_pte & (PTE_R | PTE_X)) != (PTE_R | PTE_X)) panic("kernel text permissions wrong");
    printf("  Kernel text mapping OK.\n");

    uint64_t data_start_addr = PGROUNDUP((uint64_t)etext);
    pte_t *data_pte = walk(kernel_pagetable, data_start_addr, 0);
    if(data_pte == 0 || (*data_pte & PTE_V) == 0) panic("kernel data not mapped");
    if((*data_pte & (PTE_R | PTE_W)) != (PTE_R | PTE_W)) panic("kernel data permissions wrong");
    printf("  Kernel data mapping OK.\n");

    pte_t *uart_pte = walk(kernel_pagetable, UART0, 0);
    if(uart_pte == 0 || (*uart_pte & PTE_V) == 0) panic("uart not mapped");
    if((*uart_pte & (PTE_R | PTE_W)) != (PTE_R | PTE_W)) panic("uart permissions wrong");
    printf("  UART mapping OK.\n");
    
    printf("  VM test passed.\n");
}

/**
 * @brief 测试时钟中断
 */
void test_timer_interrupt() {
    printf("--- Testing Timer Interrupt ---\n");
    printf("  (ticks 会在后台由中断自动增加)\n");
    
    int start_ticks = ticks;
    // 等待5个时钟中断发生
    while (ticks < start_ticks + 5) {
        // 使用 nop 让 CPU 忙等待
        // (现在这会在进程中运行, 不会阻塞其他进程)
        __asm__ volatile("nop");
    }
    
    printf("  Timer test passed (ticks = %d).\n", ticks);
}

/**
 * @brief 测试异常处理
 * @note 取消本函数在 kmain 中的注释会导致内核 panic (这是预期行为)。
 */
void test_exception_handling() {
    printf("\n");
    printf("--- Testing Exception Handling ---\n");
    printf("  Generating a deliberate Store Page Fault (writing to 0x0)...\n");
    
    // 这将导致 Store/AMO page fault (scause = 15)
    // 我们的 kerneltrap 将捕获它并 panic
    *((volatile char*)0x0) = 0;
    
    // 如果程序能执行到这里，说明陷阱处理失败了
    panic("Exception test FAILED! System did not trap.");
}

// (新增) C语言的 "memset"
void* memset(void *dst, int c, uint64_t n) {
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}

// (新增) C语言的 "safestrcpy"
int safestrcpy(char *s, const char *t, int n) {
  //char *os;
  //os = s;
  if (n <= 0)
    return -1;
  while (--n > 0 && (*s++ = *t++) != 0);
  *s = 0;
  return 0;
}