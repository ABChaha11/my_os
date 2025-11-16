#
# S-Mode 的中断和异常处理入口
# 由 stvec 寄存器指向
#

# .globl 声明，使其对 C 文件可见
.globl kerneltrap
.globl kernelvec

.align 4
kernelvec:
    # 任务4：上下文保存
    # 在内核栈上分配空间
    # 调整为 32 * 8 = 256
    addi sp, sp, -256

    # 保存调用者保存 (Caller-saved) 寄存器
    sd ra, 0(sp)
    sd gp, 16(sp)
    sd tp, 24(sp)   # 保存 tp
    sd t0, 32(sp)
    sd t1, 40(sp)
    sd t2, 48(sp)
    sd a0, 72(sp)
    sd a1, 80(sp)
    sd a2, 88(sp)
    sd a3, 96(sp)
    sd a4, 104(sp)
    sd a5, 112(sp)
    sd a6, 120(sp)
    sd a7, 128(sp)
    sd t3, 216(sp)
    sd t4, 224(sp)
    sd t5, 232(sp)
    sd t6, 240(sp)

    # 3. 调用 C 陷阱处理函数 (trap.c)
    call kerneltrap

    # 4. 恢复上下文 (顺序相反)
    ld ra, 0(sp)
    ld gp, 16(sp)
    ld tp, 24(sp)   # 恢复 tp
    ld t0, 32(sp)
    ld t1, 40(sp)
    ld t2, 48(sp)
    ld a0, 72(sp)
    ld a1, 80(sp)
    ld a2, 88(sp)
    ld a3, 96(sp)
    ld a4, 104(sp)
    ld a5, 112(sp)
    ld a6, 120(sp)
    ld a7, 128(sp)
    ld t3, 216(sp)
    ld t4, 224(sp)
    ld t5, 232(sp)
    ld t6, 240(sp)

    # 5. 释放栈空间
    addi sp, sp, 256

    # 6. 从 S-Mode 陷阱返回
    sret