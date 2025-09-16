# 设置 .text 段，这是存放代码的地方
.section .text

# 将 _start 符号声明为全局可见，链接器需要它作为入口点
.global _start

_start:
    # 任务1: 设置栈指针 (sp)
    # 栈的位置从内核末尾 (_end 符号) 开始，大小为 4KB * 1 (单核)
    # 将 sp 指向这块区域的顶部，因为栈是向下生长的
    .equ STACK_SIZE, 4096
    la sp, _end              # 加载内核末尾地址到 sp
    
    # --- 修正部分 ---
    # 因为 STACK_SIZE (4096) 超出了 addi 指令的12位立即数范围 (-2048 to 2047)
    # 所以先把它加载到临时寄存器 t0，再执行寄存器加法
    li t0, STACK_SIZE        # t0 = 4096
    add sp, sp, t0           # sp = sp + t0
    # --- 修正结束 ---

    #  调试代码: 验证启动和栈设置
    # 取消下面的注释来在QEMU中看到'S'和'P'字符
    # 这有助于在跳转到C之前验证汇编代码是否按预期执行
    /*
    li t0, 0x10000000   # UART 基地址
    li t1, 'S'          # 'S' for Start
    sb t1, 0(t0)        # 输出 'S'
    li t1, 'P'          # 'P' for Stack Pointer set
    sb t1, 0(t0)        # 输出 'P'
    */

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
    # 任务3: 跳转到 C 主函数
    # 使用 call 指令，它会保存返回地址并跳转
    call kmain

# 如果 kmain 意外返回，程序会进入这个无限循环，防止CPU执行未知指令
# 这是一个保护措施
spin:
    j spin

