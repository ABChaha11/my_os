# 设置 .text 段，这是存放代码的地方
.section .text

# 将 _start 符号声明为全局可见，链接器需要它作为入口点
.global _start

_start:
    # 任务1: 设置M-Mode的栈指针 (sp)
    # 我们仍然使用内核末尾的4KB空间作为初始栈
    .equ STACK_SIZE, 4096
    la sp, _end              # 加载内核末尾地址到 sp
    li t0, STACK_SIZE        # t0 = 4096
    add sp, sp, t0           # sp = sp + t0

    # 任务2: 清零 .bss 段
    # C语言要求未初始化的全局变量默认为0
    # 需要手动将 .bss 段的内存区域全部设置为0
    la t0, _bss_start   # t0 = BSS 段起始地址
    la t1, _bss_end     # t1 = BSS 段结束地址
    
bss_loop:
    beq t0, t1, bss_done # 如果 t0 == t1, 说明 BSS 已清零，跳转到 bss_done
    sd zero, 0(t0)       # 将 8 字节的 0 写入 t0 指向的地址 (sd = store doubleword)
    addi t0, t0, 8       # t0 指针前进 8 字节
    j bss_loop           # 继续循环

bss_done:
    # 任务3: 跳转到 M-Mode 的 C 语言入口 (start.c)
    call start

# 如果 start 意外返回，程序会进入这个无限循环
# 这是一个保护措施
spin:
    j spin

