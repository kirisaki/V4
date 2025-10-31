#include <stdint.h>

#include "uart.h"
#include "v4/vm_api.h"

/* Simple integer to string conversion */
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

  while (i > 0)
  {
    uart_putc(buf[--i]);
  }
}

/* V4 VM bytecode emission helpers */
static void emit8(uint8_t* code, int* k, uint8_t byte)
{
  code[(*k)++] = byte;
}

static void emit32(uint8_t* code, int* k, uint32_t val)
{
  code[(*k)++] = (val >> 0) & 0xFF;
  code[(*k)++] = (val >> 8) & 0xFF;
  code[(*k)++] = (val >> 16) & 0xFF;
  code[(*k)++] = (val >> 24) & 0xFF;
}

int main(void)
{
  uart_puts("\r\n");
  uart_puts("==================================\r\n");
  uart_puts(" V4 VM Bare-Metal Test - RISC-V\r\n");
  uart_puts("==================================\r\n\r\n");

  /* Phase 1: VM Creation */
  uart_puts("[Phase 1] Creating VM instance\r\n");

  /* Allocate memory for VM (stack-allocated to avoid malloc) */
  static uint8_t vm_mem[4096];

  VmConfig cfg;
  cfg.mem = vm_mem;
  cfg.mem_size = sizeof(vm_mem);
  cfg.mmio = NULL;
  cfg.mmio_count = 0;

  struct Vm* vm = vm_create(&cfg);
  if (!vm)
  {
    uart_puts("ERROR: Failed to create VM\r\n");
    return 1;
  }
  uart_puts("SUCCESS: VM created\r\n\r\n");

  /* Phase 2: Simple arithmetic test (10 + 32) */
  uart_puts("[Phase 2] Running bytecode: 10 + 32\r\n");

  uint8_t code[32];
  int k = 0;

  /* LIT 10 */
  emit8(code, &k, 0x00); /* Op::LIT */
  emit32(code, &k, 10);

  /* LIT 32 */
  emit8(code, &k, 0x00); /* Op::LIT */
  emit32(code, &k, 32);

  /* ADD */
  emit8(code, &k, 0x10); /* Op::ADD */

  /* RET */
  emit8(code, &k, 0x51); /* Op::RET */

  int word_idx = vm_register_word(vm, NULL, code, k);
  if (word_idx < 0)
  {
    uart_puts("ERROR: Failed to register word, error code ");
    print_int(word_idx);
    uart_puts("\r\n");
    vm_destroy(vm);
    return 1;
  }

  struct Word* word = vm_get_word(vm, word_idx);
  v4_err err = vm_exec(vm, word);
  if (err != 0)
  {
    uart_puts("ERROR: VM execution failed with error code ");
    print_int(err);
    uart_puts("\r\n");
    vm_destroy(vm);
    return 1;
  }

  /* Check result */
  int depth = vm_ds_depth_public(vm);
  if (depth != 1)
  {
    uart_puts("ERROR: Expected stack depth 1, got ");
    print_int(depth);
    uart_puts("\r\n");
    vm_destroy(vm);
    return 1;
  }

  v4_i32 result = vm_ds_peek_public(vm, 0);
  uart_puts("Result: ");
  print_int(result);
  uart_puts(" (expected 42)\r\n");

  if (result == 42)
  {
    uart_puts("SUCCESS\r\n\r\n");
  }
  else
  {
    uart_puts("FAILED\r\n\r\n");
    vm_destroy(vm);
    return 1;
  }

  /* Phase 3: Multiplication test (7 * 6) */
  uart_puts("[Phase 3] Running bytecode: 7 * 6\r\n");

  vm_reset(vm);
  k = 0;

  /* LIT 7 */
  emit8(code, &k, 0x00);
  emit32(code, &k, 7);

  /* LIT 6 */
  emit8(code, &k, 0x00);
  emit32(code, &k, 6);

  /* MUL */
  emit8(code, &k, 0x12); /* Op::MUL */

  /* RET */
  emit8(code, &k, 0x51);

  word_idx = vm_register_word(vm, NULL, code, k);
  if (word_idx < 0)
  {
    uart_puts("ERROR: Failed to register word\r\n");
    vm_destroy(vm);
    return 1;
  }

  word = vm_get_word(vm, word_idx);
  err = vm_exec(vm, word);
  if (err != 0)
  {
    uart_puts("ERROR: VM execution failed\r\n");
    vm_destroy(vm);
    return 1;
  }

  result = vm_ds_peek_public(vm, 0);
  uart_puts("Result: ");
  print_int(result);
  uart_puts(" (expected 42)\r\n");

  if (result == 42)
  {
    uart_puts("SUCCESS\r\n\r\n");
  }
  else
  {
    uart_puts("FAILED\r\n\r\n");
    vm_destroy(vm);
    return 1;
  }

  /* Cleanup */
  uart_puts("[Cleanup] Destroying VM\r\n");
  vm_destroy(vm);

  uart_puts("\r\n");
  uart_puts("==================================\r\n");
  uart_puts(" All tests passed!\r\n");
  uart_puts("==================================\r\n\r\n");

  return 0;
}
