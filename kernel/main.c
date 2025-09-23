#include "uart.h"
#include "console.h"
#include "printf.h"

// 声明由链接脚本提供的外部符号
extern void *_bss_start;
extern void *_bss_end;
extern void *_end;

// --- 修改开始: 使用 printf 来增强 panic 函数 ---
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
// --- 修改结束 ---

// --- 新增: printf 测试函数 ---
void test_printf_basic();
void test_printf_edge_cases();
// --- 新增结束 ---

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
    // 初始化串口
    uart_init();

    // 测试清屏功能
    // 打印初始信息，用于测试
    printf("--- riscv-os Kernel Booting ---\n");
    printf("Welcome! The screen is about to be cleared...\n");
    printf("Line 1: Some initial text.\n");
    printf("Line 2: More initial text.\n\n");

    // 2. 等待一段时间，让我们能清楚地看到上面的信息
    delay(3000000000UL);

    clear_screen();
    printf("Screen cleared.\n\n");

    delay(3000000000UL);

    // 运行printf测试用例
    test_printf_basic();
    printf("\n");
    test_printf_edge_cases();
    printf("\n");
    
    printf("All tests finished.\n");
    printf("Kernel is now halting.\n");

    // 内核不应该返回，进入无限循环
    while (1) {}
}


// --- 新增开始: 完整的测试用例实现 ---
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
// --- 新增结束 ---

