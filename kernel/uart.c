#include "uart.h"

// 定义 UART 寄存器的宏，方便读写
// reg 是相对于基地址 UART0 的偏移量
#define UART_REG(reg) ((volatile unsigned char *)(UART0 + reg))

// 宏的参数是一个指针，我们用 * 来解引用它，从而读写寄存器的值
#define UART_READ(reg) (*(UART_REG(reg)))
#define UART_WRITE(reg, v) (*(UART_REG(reg)) = (v))

// UART 寄存器偏移量
#define RHR 0 // 接收保持寄存器 (只读)
#define THR 0 // 发送保持寄存器 (只写)
#define LCR 3 // 线路控制寄存器
#define LSR 5 // 线路状态寄存器

// LSR 寄存器的位定义
#define LSR_TX_IDLE (1 << 5) // 发送保持寄存器为空 (Transmitter holding register is empty)

/**
 * @brief 初始化 UART
 */
void uart_init(void) {
    // 1. 设置波特率
    //    首先，向 LCR 写入 0x80，使其 DLAB 位为1，这样才能访问波特率除数锁存器
    UART_WRITE(LCR, 0x80);
    //    写入除数的低8位和高8位。0x03 -> 波特率 38.4k
    UART_WRITE(0, 0x03);
    UART_WRITE(1, 0x00);

    // 2. 设置数据格式
    //    将 LCR 设置为 0x03，表示8位数据，无校验，1位停止位 (8-N-1)
    //    同时 DLAB 位被清零，UART 恢复正常模式
    UART_WRITE(LCR, 0x03);

    // 3. (可选但推荐) 启用 FIFO
    //    向 FCR 写入 0x07 来启用并重置 FIFO
    UART_WRITE(2, 0x07);
}

/**
 * @brief 通过 UART 发送一个字符
 * @param c 要发送的字符
 */
void uart_putc(char c) {
    // 等待，直到发送保持寄存器为空
    // LSR 的第 5 位 (LSR_TX_IDLE) 为 1 时，表示 THR 为空，可以发送下一个字符
    // 这就是所谓的“轮询”方式
    while ((UART_READ(LSR) & LSR_TX_IDLE) == 0);

    // 向 THR 写入字符，硬件会自动将其发送出去
    UART_WRITE(THR, c);
}

/**
 * @brief 通过 UART 发送一个以 null 结尾的字符串
 * @param s 要发送的字符串指针
 */
void uart_puts(char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}
