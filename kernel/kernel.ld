OUTPUT_ARCH("riscv")
ENTRY(_start)

SECTIONS
{
. = 0x80000000;

.text : {
    *(.text .text.*)
}
PROVIDE(etext = .);

.rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*)
    *(.rodata .rodata.*)
}
PROVIDE(erodata = .);

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
