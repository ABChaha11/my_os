OUTPUT_ARCH("riscv")
ENTRY(_start)

SECTIONS
{
. = 0x80000000;

.text : {
    *(.text .text.*)
}

.rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*)
    *(.rodata .rodata.*)
<<<<<<< HEAD
=======
    PROVIDE(etext = .);
>>>>>>> b9fe6fb (实验3：页表与内存管理)
}

.data : {
    . = ALIGN(16);
    *(.sdata .sdata.*)
    *(.data .data.*)
}

.bss : {
    . = ALIGN(16);
    PROVIDE(_bss_start = .);
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    PROVIDE(_bss_end = .);
}

PROVIDE(_end = .);

}
