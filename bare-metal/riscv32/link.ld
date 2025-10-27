/* Linker script for RISC-V 32-bit bare-metal on QEMU virt machine */

OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    /* QEMU RISC-V virt machine RAM starts at 0x80000000 */
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
    . = 0x80000000;

    .text : {
        *(.text.init)
        *(.text .text.*)
    } > RAM

    .rodata : {
        *(.rodata .rodata.*)
    } > RAM

    .data : {
        *(.data .data.*)
        *(.sdata .sdata.*)
    } > RAM

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        __bss_end = .;
    } > RAM

    /* Stack grows downward from end of RAM */
    . = ALIGN(16);
    __heap_start = .;
    . = 0x80000000 + 16M;  /* Reserve 16MB for program */
    __stack_top = .;

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}
