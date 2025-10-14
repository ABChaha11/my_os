#include "uart.h"
#include "console.h"
#include "printf.h"
#include "pmm.h"
#include "vm.h"
#include "riscv.h"
#include "memlayout.h"
#include "defs.h"

// 声明由链接脚本提供的外部符号
extern void *_bss_start;
extern void *_bss_end;
extern void *_end;

/**
 * @brief 内核致命错误处理函数
 * @param s 错误信息字符串
 */
void panic(char *s){
    // 使用我们强大的新printf函数
    printf("PANIC: %s\n", s);
    // 锁定系统，进入无限循环
    while (1) {}
}

// --- 测试函数声明 ---
void test_printf_basic();
void test_printf_edge_cases();
void test_pmm();
void test_vm();

/**
 * @brief 一个简单的忙等待延时函数
 * @param count 延时计数值。这是一个大约值，需要足够大才能看到明显效果。
 * @note 'volatile' 关键字是必须的，它会告诉编译器不要“优化掉”这个看起来没用的循环。
 */
static void delay(volatile unsigned long count) {
    // 这个循环会一直执行，直到count减到0为止，从而达到延时的效果。
    while (count--){
        __asm__ volatile("nop");
    }
}

/**
 * @brief 内核的C语言入口函数
 */
void kmain(void) {
    // 1. 初始化串口
    uart_init();
    printf("--- riscv-os Kernel Booting ---\n");
    // 2. 初始化物理内存管理器
    pmm_init();
    
    // 3. 创建内核页表并进行映射
    kvminit();

    // 4. 激活虚拟内存 (启用分页)
    kvminithart();

    // 等待一段时间，让我们能清楚地看到上面的信息
    delay(3000000000UL);

    clear_screen();
    printf("Screen cleared.\n\n");

    delay(3000000000UL);

    // --- 运行测试 ---
    printf("\n--- Running Tests ---\n");

    test_printf_basic();
    printf("\n");
    test_printf_edge_cases();
    printf("\n");
    
    test_pmm();
    test_vm();

    printf("\n--- All tests passed! ---\n");

    printf("Kernel is now halting.\n");

    // 内核不应该返回，进入无限循环
    while (1) {}
}


// ---完整的测试用例实现 ---
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

    pte_t *text_pte = walk_lookup(kernel_pagetable, KERNBASE);
    if(text_pte == 0 || (*text_pte & PTE_V) == 0) panic("kernel text not mapped");
    if((*text_pte & (PTE_R | PTE_X)) != (PTE_R | PTE_X)) panic("kernel text permissions wrong");
    printf("  Kernel text mapping OK.\n");

    uint64_t data_start_addr = PGROUNDUP((uint64_t)etext);
    pte_t *data_pte = walk_lookup(kernel_pagetable, data_start_addr);
    if(data_pte == 0 || (*data_pte & PTE_V) == 0) panic("kernel data not mapped");
    if((*data_pte & (PTE_R | PTE_W)) != (PTE_R | PTE_W)) panic("kernel data permissions wrong");
    printf("  Kernel data mapping OK.\n");

    pte_t *uart_pte = walk_lookup(kernel_pagetable, UART0);
    if(uart_pte == 0 || (*uart_pte & PTE_V) == 0) panic("uart not mapped");
    if((*uart_pte & (PTE_R | PTE_W)) != (PTE_R | PTE_W)) panic("uart permissions wrong");
    printf("  UART mapping OK.\n");
    
    printf("  VM test passed.\n");
}

