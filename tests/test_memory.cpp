#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <string.h>

#include "doctest.h"
#include "v4/internal/vm.h"
#include "v4/vm_api.h"

/* ------------------------------------------------------------------------- */
/* Dummy MMIO handlers                                                       */
/* ------------------------------------------------------------------------- */
/**
 * A minimal MMIO device for testing.
 * - read32 returns a fixed pattern (0xAABBCCDD + addr low nibble)
 * - write32 stores the last written address and value
 */
struct Dummy
{
  v4_u32 last_write_addr = 0;
  v4_u32 last_write_val = 0;
};

static v4_err d_read32(void *user, v4_u32 addr, v4_u32 *out)
{
  (void)user;
  *out = 0xAABBCCDDu + (addr & 0xFu);
  return 0;
}

static v4_err d_write32(void *user, v4_u32 addr, v4_u32 val)
{
  Dummy *d = (Dummy *)user;
  d->last_write_addr = addr;
  d->last_write_val = val;
  return 0;
}

/* ------------------------------------------------------------------------- */
/* Test cases                                                                */
/* ------------------------------------------------------------------------- */

/**
 * @test RAM normal path: STORE → LOAD should return the same value.
 */
TEST_CASE("RAM basic read/write round-trip")
{
  uint8_t ram[64] = {};
  VmConfig cfg{ram, (v4_u32)sizeof(ram), nullptr, 0};
  Vm *vm = vm_create(&cfg);
  REQUIRE(vm);

  v4_err e;
  e = vm_mem_write32(vm, 0, 0x12345678u);
  CHECK(e == 0);

  v4_u32 out = 0;
  e = vm_mem_read32(vm, 0, &out);
  CHECK(e == 0);
  CHECK(out == 0x12345678u);

  // Verify little-endian layout in memory
  CHECK(ram[0] == 0x78);
  CHECK(ram[1] == 0x56);
  CHECK(ram[2] == 0x34);
  CHECK(ram[3] == 0x12);

  vm_destroy(vm);
}

/**
 * @test Out-of-bounds check:
 *       addr = mem_size - 4 → OK
 *       addr = mem_size - 3 → should fail with -13 (OobMemory)
 */
TEST_CASE("Out-of-bounds access handling")
{
  uint8_t ram[16] = {};
  VmConfig cfg{ram, 16u, nullptr, 0};
  Vm *vm = vm_create(&cfg);
  REQUIRE(vm);

  // Last aligned word should be accessible
  v4_err e = vm_mem_write32(vm, 12u, 0xDEADBEEFu);
  CHECK(e == 0);

  // Crossing the end of memory should fail
  v4_u32 out = 0;
  e = vm_mem_read32(vm, 13u, &out);
  CHECK(e == -13);  // OobMemory

  vm_destroy(vm);
}

/**
 * @test Unaligned address should raise -12 (Unaligned).
 */
TEST_CASE("Unaligned access detection")
{
  uint8_t ram[16] = {};
  VmConfig cfg{ram, 16u, nullptr, 0};
  Vm *vm = vm_create(&cfg);
  REQUIRE(vm);

  v4_err e = vm_mem_write32(vm, 2u, 0);
  CHECK(e == -12);  // Unaligned

  vm_destroy(vm);
}

/**
 * @test Verify that MMIO read/write callbacks are invoked correctly.
 */
TEST_CASE("MMIO callback dispatch")
{
  uint8_t ram[64] = {};
  Dummy dummy{};
  V4_Mmio m{/* base */ 0x20u,
            /* size */ 0x10u,
            /* rd/wr */ d_read32, d_write32,
            /* user */ &dummy};
  VmConfig cfg{ram, (v4_u32)sizeof(ram), &m, 1};
  Vm *vm = vm_create(&cfg);
  REQUIRE(vm);

  // Write should go to MMIO
  v4_err e = vm_mem_write32(vm, 0x24u, 0xCAFEBABEu);
  CHECK(e == 0);
  CHECK(dummy.last_write_addr == 0x24u);
  CHECK(dummy.last_write_val == 0xCAFEBABEu);

  // Read should also go to MMIO
  v4_u32 out = 0;
  e = vm_mem_read32(vm, 0x28u, &out);
  CHECK(e == 0);
  CHECK(out == 0xAABBCCDDu + (0x28u & 0xFu));

  vm_destroy(vm);
}

/**
 * @test Combined scenario: alternating between RAM and MMIO regions.
 */
TEST_CASE("Mixed RAM/MMIO access")
{
  uint8_t ram[64] = {};
  Dummy dummy{};
  V4_Mmio m{0x10u, 0x10u, d_read32, d_write32, &dummy};
  VmConfig cfg{ram, (v4_u32)sizeof(ram), &m, 1};
  Vm *vm = vm_create(&cfg);
  REQUIRE(vm);

  // 1. RAM region
  CHECK(vm_mem_write32(vm, 0x00u, 0x01020304u) == 0);

  // 2. MMIO region
  CHECK(vm_mem_write32(vm, 0x14u, 0x11111111u) == 0);
  v4_u32 v = 0;
  CHECK(vm_mem_read32(vm, 0x18u, &v) == 0);

  // 3. Back to RAM region
  CHECK(vm_mem_write32(vm, 0x2Cu, 0xFFEEDDCCu) == 0);
  CHECK(vm_mem_read32(vm, 0x2Cu, &v) == 0);
  CHECK(v == 0xFFEEDDCCu);

  vm_destroy(vm);
}
