#include "uart.h" // 控制台层依赖硬件抽象层
#include "console.h"

/**
 * @brief 向控制台输出一个字符
 * 这是所有输出的基础，目前它只是简单地封装了uart_putc
 * @param c 要输出的字符
 */
void console_putc(char c) {
    uart_putc(c);
}

/**
 * @brief 向控制台输出一个字符串
 * @param s 要输出的字符串
 */
void console_puts(const char *s) {
    while(*s) {
        console_putc(*s);
        s++;
    }
}

/**
 * @brief 清空屏幕并移动光标到左上角
 * @note 使用 ANSI VT100 转义序列。
 * 这个新版本逐个字符地、显式地发送控制代码，以确保最大的兼容性和可靠性。
 */
void clear_screen(void) {
    console_puts("\x1b[2J\x1b[H");
}

