#include "console.h" // 我们将依赖控制台层
#include <stdarg.h>  // 用于处理可变参数
#include <stddef.h>  // for NULL

// 静态辅助函数，不对外暴露
static void print_number(long long num, int base, int sign, int width, char padc);
static void print_string(char *s);

/**
 * @brief 格式化输出到内存缓冲区
 * @param out 指向目标缓冲区的指针
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 写入的字符数
 */
int sprintf(char *out, const char *format, ...) {
    // sprintf 的实现留作扩展练习，本次实验我们专注于printf
    // 一个简单的实现思路是：修改所有 putc/puts 函数，让它们
    // 写入到 out 缓冲区而不是控制台。
    return 0;
}


/**
 * @brief 格式化输出到控制台
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 写入的字符数
 */
int printf(const char *format, ...) {
    va_list ap;
    int i = 0;
    char *s;
    long long num;
    int long_flag;

    va_start(ap, format);
    while (format && format[i]) {
        char c = format[i];
        if (c != '%') {
            console_putc(c);
            i++;
            continue;
        }
        
        i++; // 跳过 '%'
        c = format[i];

        long_flag = 0;
        if (c == 'l') {
            long_flag = 1;
            i++;
            c = format[i];
        }

        switch (c) {
            case 'd':
                num = long_flag ? va_arg(ap, long) : va_arg(ap, int);
                print_number(num, 10, 1, 0, ' ');
                break;
            case 'x':
            case 'p':
                num = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
                if (c == 'p') {
                    console_puts("0x");
                }
                print_number(num, 16, 0, 0, ' ');
                break;
            case 's':
                s = va_arg(ap, char *);
                if (s == NULL) {
                    s = "(null)";
                }
                print_string(s);
                break;
            case 'c':
                console_putc(va_arg(ap, int));
                break;
            case '%':
                console_putc('%');
                break;
            case '\0':
                goto out;
            default:
                console_putc('%');
                console_putc(c);
                break;
        }
        i++;
    }

out:
    va_end(ap);
    return 0;
}

/**
 * @brief 输出一个字符串
 */
static void print_string(char *s) {
    while(*s) {
        console_putc(*s);
        s++;
    }
}

/**
 * @brief 核心数字转换算法
 * @param num 要转换的数字
 * @param base 进制 (10 或 16)
 * @param sign 是否为有符号数 (1 是, 0 否)
 */
static void print_number(long long num, int base, int sign, int width, char padc) {
    static char digits[] = "0123456789abcdef";
    char buf[40];
    int i = 0, is_neg = 0;
    unsigned long long n;

    if (sign && num < 0) {
        is_neg = 1;
        n = -num;
    } else {
        n = num;
    }

    if (n == 0) {
        buf[i++] = '0';
    } else {
        while (n > 0) {
            buf[i++] = digits[n % base];
            n /= base;
        }
    }
    
    if (is_neg) {
        console_putc('-');
    }

    while (--i >= 0) {
        console_putc(buf[i]);
    }
}

