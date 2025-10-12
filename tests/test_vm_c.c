// tests/test_vm_c.c — public C API only (no internal headers)
#include <stdint.h>
#include <stdio.h>

#include "v4/vm_api.h"

int main(void)
{
  uint8_t ram[32] = {0};

  // Configure a VM with 32 bytes of RAM and no MMIO windows.
  VmConfig cfg;
  cfg.mem = ram;
  cfg.mem_size = (v4_u32)sizeof(ram);
  cfg.mmio = NULL;
  cfg.mmio_count = 0;

  struct Vm *vm = vm_create(&cfg);
  if (!vm)
  {
    fprintf(stderr, "vm_create failed\n");
    return 1;
  }

  // Optional: reset stacks/state
  vm_reset(vm);

  // ------ RAM write/read (aligned, in-bounds) ------
  v4_err e;
  v4_u32 out = 0;

  e = vm_mem_write32(vm, 0u, 0x11223344u);
  if (e != 0)
  {
    fprintf(stderr, "vm_mem_write32 aligned failed: %d\n", e);
    vm_destroy(vm);
    return 2;
  }

  e = vm_mem_read32(vm, 0u, &out);
  if (e != 0 || out != 0x11223344u)
  {
    fprintf(stderr, "vm_mem_read32 aligned mismatch: err=%d val=0x%08x\n", e,
            (unsigned)out);
    vm_destroy(vm);
    return 3;
  }

  // ------ Unaligned access should fail with -12 (Unaligned) ------
  e = vm_mem_write32(vm, 2u, 0u);
  if (e != -12)
  {
    fprintf(stderr, "unaligned write should be -12, got %d\n", e);
    vm_destroy(vm);
    return 4;
  }

  // ------ Out-of-bounds should fail with -13 (OobMemory) ------
  e = vm_mem_read32(vm, (v4_u32)(cfg.mem_size - 3u), &out);  // crosses the end
  if (e != -13)
  {
    fprintf(stderr, "oob read should be -13, got %d\n", e);
    vm_destroy(vm);
    return 5;
  }

  // If we reached here, basic public API checks passed.
  vm_destroy(vm);
  printf("test_vm_c: OK\n");
  return 0;
}
