#pragma once

#include <stdint.h>

/* QEMU RISC-V virt machine UART (16550 compatible) */
#define UART_BASE 0x10000000

/* UART registers */
#define UART_THR (UART_BASE + 0) /* Transmit Holding Register */
#define UART_LSR (UART_BASE + 5) /* Line Status Register */

/* LSR bits */
#define UART_LSR_THRE (1 << 5) /* Transmit Holding Register Empty */

static inline void uart_putc(char c)
{
  volatile uint8_t* thr = (volatile uint8_t*)UART_THR;
  *thr = c;
}

static inline void uart_puts(const char* s)
{
  while (*s)
  {
    uart_putc(*s++);
  }
}
