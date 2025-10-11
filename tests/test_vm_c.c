#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <v4/vm_api.h>
#include <v4/opcodes.h>

static void le32(uint8_t *p, int32_t v)
{
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)((v >> 8) & 0xFF);
  p[2] = (uint8_t)((v >> 16) & 0xFF);
  p[3] = (uint8_t)((v >> 24) & 0xFF);
}

int main(void)
{
  assert(v4_vm_version() == 0);

  // LIT 10; LIT 20; ADD; RET
  uint8_t code[1 + 4 + 1 + 4 + 1 + 1];
  int k = 0;
  code[k++] = (uint8_t)V4_OP_LIT;
  le32(&code[k], 10);
  k += 4;
  code[k++] = (uint8_t)V4_OP_LIT;
  le32(&code[k], 20);
  k += 4;
  code[k++] = (uint8_t)V4_OP_ADD;
  code[k++] = (uint8_t)V4_OP_RET;

  Vm vm;
  vm_reset(&vm);
  int rc = vm_exec(&vm, code, k);
  assert(rc == 0);
  assert(vm.sp == vm.DS + 1);
  assert(vm.DS[0] == 30);

  puts("C smoke test OK");
  return 0;
}
