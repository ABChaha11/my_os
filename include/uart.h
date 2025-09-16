#ifndef __UART_H__
#define __UART_H__

// QEMU virt 机器上 UART 的内存映射基地址
#define UART0 0x10000000L

// 初始化 UART
void uart_init(void);

// 通过 UART 发送一个字符
void uart_putc(char c);

// 通过 UART 发送一个以 null 结尾的字符串
void uart_puts(char *s);

#endif // __UART_H__
