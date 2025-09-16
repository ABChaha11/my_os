#include "uart.h"

// 声明由链接脚本提供的外部符号
// 这些符号是地址，而不是变量的值
extern void *_bss_start;
extern void *_bss_end;
extern void *_end;

/**
 * @brief 内核的C语言入口函数
 */

void panic(char *s){
    //打印错误信息
    uart_puts("PANIC:");
    uart_puts(s);
    uart_puts("\n");

    //锁定系统，进入无限循环
    while (1) {}
}

void kmain(void) {
    // 注意：BSS段的清零已经在 entry.S 中完成了
    // 如果要在C中实现，可以像下面这样：
    // for (char *p = (char*)&_bss_start; p < (char*)&_bss_end; ++p) {
    //     *p = 0;
    // }

    // 初始化串口
    uart_init();

    // 输出 "Hello OS"
    uart_puts("Hello OS\n");

    // --- 测试 panic 函数 ---
    // 下面的代码将主动触发 panic，以演示错误处理机制
    // 在真实的操作系统中，你会在无法恢复的错误发生时调用它
    /*panic("End of boot test");*/

    // 内核不应该返回，进入无限循环
    while (1) {
        // hlt or other power-saving instructions can be used here
    }
}
