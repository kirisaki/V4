#pragma once
#include "v4/internal/vm.h"
#include "v4/vm_api.h"

/**
 * Core memory helpers used by LOAD/STORE and public API wrappers.
 *  - 4-byte aligned access only
 *  - Little-endian layout
 *  - Out-of-range and MMIO handled in software
 */

v4_err v4_mem_read32_core(Vm *vm, v4_u32 addr, v4_u32 *out);
v4_err v4_mem_write32_core(Vm *vm, v4_u32 addr, v4_u32 val);

v4_err v4_mem_read8_core(Vm *vm, v4_u32 addr, v4_u32 *out);
v4_err v4_mem_read16_core(Vm *vm, v4_u32 addr, v4_u32 *out);
v4_err v4_mem_write8_core(Vm *vm, v4_u32 addr, v4_u32 val);
v4_err v4_mem_write16_core(Vm *vm, v4_u32 addr, v4_u32 val);

/* Alignment check (4-byte). Returns 0 or -12 (Unaligned). */
static inline v4_err v4_is_aligned4(v4_u32 addr)
{
  return (addr & 3u) == 0u ? 0 : -12;
}

/* Range check within RAM. Returns 0 or -13 (OobMemory). */
static inline v4_err v4_is_in_ram(Vm *vm, v4_u32 addr, v4_u32 bytes)
{
  uint64_t b = (uint64_t)addr + (uint64_t)bytes;
  return (vm->mem && b <= (uint64_t)vm->mem_size) ? 0 : -13;
}
