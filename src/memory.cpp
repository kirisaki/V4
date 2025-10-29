// src/memory.cpp — public API wrappers + core memory ops (Tier-0)
#include "v4/internal/memory.hpp"  // v4_is_aligned4 / v4_is_in_ram + prototypes

#include <stdlib.h>
#include <string.h>

#include "v4/internal/vm.h"  // internal Vm definition (your repo)
#include "v4/vm_api.h"

/* ---- Little-endian helpers ---- */
static inline v4_u32 ld_le32(const uint8_t *p)
{
  return (v4_u32)p[0] | ((v4_u32)p[1] << 8) | ((v4_u32)p[2] << 16) | ((v4_u32)p[3] << 24);
}
static inline void st_le32(uint8_t *p, v4_u32 v)
{
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)((v >> 8) & 0xFF);
  p[2] = (uint8_t)((v >> 16) & 0xFF);
  p[3] = (uint8_t)((v >> 24) & 0xFF);
}
static inline uint16_t ld_le16(const uint8_t *p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline void st_le16(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)((v >> 8) & 0xFF);
}

/* ---- MMIO window lookup (linear) ---- */
static int find_mmio(Vm *vm, v4_u32 addr)
{
  for (int i = 0; i < vm->mmio_count; ++i)
  {
    const v4_u32 base = vm->mmio[i].base;
    const v4_u32 size = vm->mmio[i].size;
    if (addr >= base && (addr - base) < size)
      return i;
  }
  return -1;
}

/* ---- core 32-bit accessors (used by VM ops and public API) ---- */
v4_err v4_mem_read32_core(Vm *vm, v4_u32 addr, v4_u32 *out)
{
  // 1) MMIO window?
  const int mi = find_mmio(vm, addr);
  if (mi >= 0)
  {
    // (Option) For MMIO we can keep alignment check; but OOB must not shadow it.
    if (int e = v4_is_aligned4(addr))
      return e;
    const V4_Mmio *m = &vm->mmio[mi];
    if (!m->read32)
      return -13;  // forbidden -> treat as OOB
    return m->read32(m->user, addr, out);
  }

  // 2) RAM range check FIRST (so OOB wins over Unaligned)
  if (int e = v4_is_in_ram(vm, addr, 4))
    return e;

  // 3) Alignment check for valid RAM
  if (int e = v4_is_aligned4(addr))
    return e;

  // 4) Load
  *out = ld_le32(&vm->mem[addr]);
  return 0;
}

v4_err v4_mem_write32_core(Vm *vm, v4_u32 addr, v4_u32 val)
{
  // 1) MMIO window?
  const int mi = find_mmio(vm, addr);
  if (mi >= 0)
  {
    if (int e = v4_is_aligned4(addr))
      return e;
    const V4_Mmio *m = &vm->mmio[mi];
    if (!m->write32)
      return -13;  // forbidden
    return m->write32(m->user, addr, val);
  }

  // 2) RAM range check FIRST
  if (int e = v4_is_in_ram(vm, addr, 4))
    return e;

  // 3) Alignment check for valid RAM
  if (int e = v4_is_aligned4(addr))
    return e;

  // 4) Store
  st_le32(&vm->mem[addr], val);
  return 0;
}

/* ---- 8-bit and 16-bit accessors ---- */
v4_err v4_mem_read8_core(Vm *vm, v4_u32 addr, v4_u32 *out)
{
  // No MMIO support for 8-bit access (could be added if needed)
  // RAM range check
  if (int e = v4_is_in_ram(vm, addr, 1))
    return e;

  // Load byte (unsigned)
  *out = (v4_u32)vm->mem[addr];
  return 0;
}

v4_err v4_mem_read16_core(Vm *vm, v4_u32 addr, v4_u32 *out)
{
  // No MMIO support for 16-bit access (could be added if needed)
  // RAM range check
  if (int e = v4_is_in_ram(vm, addr, 2))
    return e;

  // No alignment requirement for 16-bit access
  // Load 16-bit (unsigned, little-endian)
  *out = (v4_u32)ld_le16(&vm->mem[addr]);
  return 0;
}

v4_err v4_mem_write8_core(Vm *vm, v4_u32 addr, v4_u32 val)
{
  // No MMIO support for 8-bit access
  // RAM range check
  if (int e = v4_is_in_ram(vm, addr, 1))
    return e;

  // Store byte
  vm->mem[addr] = (uint8_t)(val & 0xFF);
  return 0;
}

v4_err v4_mem_write16_core(Vm *vm, v4_u32 addr, v4_u32 val)
{
  // No MMIO support for 16-bit access
  // RAM range check
  if (int e = v4_is_in_ram(vm, addr, 2))
    return e;

  // No alignment requirement for 16-bit access
  // Store 16-bit (little-endian)
  st_le16(&vm->mem[addr], (uint16_t)(val & 0xFFFF));
  return 0;
}

/* ---- public API: direct memory access (for tests/embedding) ---- */
extern "C" v4_err vm_mem_read32(struct Vm *vmp, v4_u32 addr, v4_u32 *out)
{
  Vm *vm = (Vm *)vmp;
  v4_err e = v4_mem_read32_core(vm, addr, out);
  vm->last_err = e;
  return e;
}
extern "C" v4_err vm_mem_write32(struct Vm *vmp, v4_u32 addr, v4_u32 val)
{
  Vm *vm = (Vm *)vmp;
  v4_err e = v4_mem_write32_core(vm, addr, val);
  vm->last_err = e;
  return e;
}

/* ---- public API: lifecycle & MMIO registration ---- */
extern "C" struct Vm *vm_create(const VmConfig *cfg)
{
  if (!cfg)
    return nullptr;
  Vm *vm = (Vm *)::malloc(sizeof(Vm));
  if (!vm)
    return nullptr;
  ::memset(vm, 0, sizeof(Vm));

  // Stacks
  vm_reset(vm);  // declared in vm_api.h / defined in vm_core.cpp

  // Memory config
  vm->mem = cfg->mem;
  vm->mem_size = cfg->mem_size;

  // MMIO table (linear, fixed capacity)
  vm->mmio_count = 0;
  if (cfg->mmio && cfg->mmio_count > 0)
  {
    const int cap = Vm::V4_MAX_MMIO;  // from internal vm.h
    const int n = (cfg->mmio_count <= cap) ? cfg->mmio_count : cap;
    for (int i = 0; i < n; ++i)
      vm->mmio[i] = cfg->mmio[i];
    vm->mmio_count = n;
  }

  // Arena allocator (optional)
  vm->arena = cfg->arena;

  vm->last_err = 0;
  vm->boot_cfg_snapshot = cfg;
  return vm;
}

extern "C" void vm_destroy(struct Vm *vm)
{
  if (!vm)
    return;

  // Free word names only if using malloc (not arena)
  if (!vm->arena)
  {
    for (int i = 0; i < vm->word_count; i++)
    {
      if (vm->words[i].name)
      {
        ::free(vm->words[i].name);
        vm->words[i].name = nullptr;
      }
    }
  }
  // If using arena, names are managed by arena owner (user responsibility)

  ::free(vm);
}

extern "C" v4_err vm_register_mmio(struct Vm *vmp, const V4_Mmio *list, int count)
{
  Vm *vm = (Vm *)vmp;
  if (!vm || !list || count <= 0)
    return -1;
  int appended = 0;
  for (int i = 0; i < count; ++i)
  {
    if (vm->mmio_count >= Vm::V4_MAX_MMIO)
      break;
    vm->mmio[vm->mmio_count++] = list[i];
    ++appended;
  }
  return (appended == count) ? 0 : -1;
}
