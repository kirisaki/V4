/* RISC-V 32-bit startup code for bare-metal */

.section .text.init
.global _start
.type _start, @function

_start:
    /* Disable interrupts */
    csrw mie, zero

    /* Set up stack pointer */
    la sp, __stack_top

    /* Clear BSS section */
    la a0, __bss_start
    la a1, __bss_end
1:
    bge a0, a1, 2f
    sw zero, 0(a0)
    addi a0, a0, 4
    j 1b
2:

    /* Call main function */
    call main

    /* If main returns, loop forever */
halt:
    wfi
    j halt

.size _start, . - _start
