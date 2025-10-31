#include <stdint.h>

#include "uart.h"

/* Simple integer to string conversion (for positive numbers only) */
static void print_int(int32_t n)
{
  if (n == 0)
  {
    uart_putc('0');
    return;
  }

  if (n < 0)
  {
    uart_putc('-');
    n = -n;
  }

  char buf[12];
  int i = 0;

  while (n > 0)
  {
    buf[i++] = '0' + (n % 10);
    n /= 10;
  }

  /* Print in reverse order */
  while (i > 0)
  {
    uart_putc(buf[--i]);
  }
}

int main(void)
{
  uart_puts("V4 VM Bare-Metal Test\r\n");
  uart_puts("=====================\r\n\r\n");

  uart_puts("Phase 1: Basic UART output\r\n");
  uart_puts("Hello from RISC-V bare-metal!\r\n\r\n");

  uart_puts("Phase 2: Simple calculations\r\n");
  int32_t a = 10;
  int32_t b = 32;
  int32_t result = a + b;

  uart_puts("10 + 32 = ");
  print_int(result);
  uart_puts("\r\n");

  result = a * b;
  uart_puts("10 * 32 = ");
  print_int(result);
  uart_puts("\r\n\r\n");

  uart_puts("Test completed successfully!\r\n");

  return 0;
}
