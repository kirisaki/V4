#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cstdint>
#include "v4/vm_api.h"
#include "v4/opcodes.hpp"
#include "v4/errors.hpp"
#include "v4/internal/vm.h"

/* ------------------------------------------------------------------------- */
/* Bytecode emit helpers                                                     */
/* ------------------------------------------------------------------------- */
static inline void emit8(uint8_t *code, int *k, uint8_t v)
{
  code[(*k)++] = v;
}
static inline void emit16(uint8_t *code, int *k, int16_t v)
{
  code[(*k)++] = (uint8_t)(v & 0xFF);
  code[(*k)++] = (uint8_t)((v >> 8) & 0xFF);
}
static inline void emit32(uint8_t *code, int *k, int32_t v)
{
  code[(*k)++] = (uint8_t)(v & 0xFF);
  code[(*k)++] = (uint8_t)((v >> 8) & 0xFF);
  code[(*k)++] = (uint8_t)((v >> 16) & 0xFF);
  code[(*k)++] = (uint8_t)((v >> 24) & 0xFF);
}

/* ------------------------------------------------------------------------- */
/* Smoke: version                                                            */
/* ------------------------------------------------------------------------- */
TEST_CASE("vm version")
{
  CHECK(v4_vm_version() == 0);
}

/* ------------------------------------------------------------------------- */
/* Arithmetic and stack operation tests                                      */
/* ------------------------------------------------------------------------- */
TEST_CASE("basic stack ops (LIT/SWAP/DUP/OVER/DROP/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 1, 0, 0, 0,
      (uint8_t)Op::LIT, 2, 0, 0, 0,
      (uint8_t)Op::SWAP,
      (uint8_t)Op::DUP,
      (uint8_t)Op::OVER,
      (uint8_t)Op::DROP,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 3);
  CHECK(vm.DS[0] == 2);
  CHECK(vm.DS[1] == 1);
}

TEST_CASE("basic arithmetic (LIT/ADD/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::LIT, 20, 0, 0, 0,
      (uint8_t)Op::ADD,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 30);
}

TEST_CASE("subtraction (LIT/SUB/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 20, 0, 0, 0,
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::SUB,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 10);
}

TEST_CASE("multiplication (LIT/MUL/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 6, 0, 0, 0,
      (uint8_t)Op::LIT, 7, 0, 0, 0,
      (uint8_t)Op::MUL,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 42);
}

TEST_CASE("division (LIT/DIV/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 42, 0, 0, 0,
      (uint8_t)Op::LIT, 7, 0, 0, 0,
      (uint8_t)Op::DIV,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 6);
}

TEST_CASE("modulus (LIT/MOD/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 43, 0, 0, 0,
      (uint8_t)Op::LIT, 7, 0, 0, 0,
      (uint8_t)Op::MOD,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 1);
}

TEST_CASE("error: division by zero")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 42, 0, 0, 0,
      (uint8_t)Op::LIT, 0, 0, 0, 0,
      (uint8_t)Op::DIV,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == static_cast<int>(Err::DivByZero));
  CHECK(vm.sp == vm.DS + 0);
}

/* ------------------------------------------------------------------------- */
/* Comparison                                                                */
/* ------------------------------------------------------------------------- */
TEST_CASE("equality (LIT/EQ/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::EQ,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/NE/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::LIT, 20, 0, 0, 0,
      (uint8_t)Op::NE,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/GT/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 20, 0, 0, 0,
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::GT,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/GE/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::GE,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/LT/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::LIT, 20, 0, 0, 0,
      (uint8_t)Op::LT,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/LE/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::LE,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

/* ------------------------------------------------------------------------- */
/* Bitwise                                                                   */
/* ------------------------------------------------------------------------- */
TEST_CASE("bitwise AND (LIT/AND/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 0b1100, 0, 0, 0,
      (uint8_t)Op::LIT, 0b1010, 0, 0, 0,
      (uint8_t)Op::AND,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 0b1000);
}

TEST_CASE("bitwise OR (LIT/OR/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 0b1100, 0, 0, 0,
      (uint8_t)Op::LIT, 0b1010, 0, 0, 0,
      (uint8_t)Op::OR,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 0b1110);
}

TEST_CASE("bitwise INVERT (LIT/INVERT/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 0b1100, 0, 0, 0,
      (uint8_t)Op::INVERT,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == ~0b1100);
}

TEST_CASE("bitwise XOR (LIT/XOR/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 0b1100, 0, 0, 0,
      (uint8_t)Op::LIT, 0b1010, 0, 0, 0,
      (uint8_t)Op::XOR,
      (uint8_t)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 0b0110);
}

/* ------------------------------------------------------------------------- */
/* Control flow                                                              */
/* ------------------------------------------------------------------------- */
TEST_CASE("unconditional branch (JMP)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 1, 0, 0, 0, // 0: DS[0]=1
      (uint8_t)Op::JMP, 5, 0,       // 5: jump to 12
      (uint8_t)Op::LIT, 2, 0, 0, 0, // 7: DS[0]=2 (skipped)
      (uint8_t)Op::RET              // 12:
  };
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 1);
}

TEST_CASE("conditional branch (JZ)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 0, 0, 0, 0, // 0: DS[0]=0
      (uint8_t)Op::JZ, 8, 0,        // 5: jump to 14 if DS[0]==0
      (uint8_t)Op::LIT, 1, 0, 0, 0, // 7: DS[0]=1 (skipped)
      (uint8_t)Op::JMP, 6, 0,       // 12: jump to 20
      (uint8_t)Op::LIT, 2, 0, 0, 0, // 14: DS[0]=2
      (uint8_t)Op::RET              // 19:
  };
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 2);
}

TEST_CASE("conditional branch (JNZ)")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 1, 0, 0, 0, // 0: DS[0]=1
      (uint8_t)Op::JNZ, 8, 0,       // 5: jump to 14 if DS[0]!=0
      (uint8_t)Op::LIT, 2, 0, 0, 0, // 7: DS[0]=2 (skipped)
      (uint8_t)Op::JMP, 6, 0,       // 12: jump to 20
      (uint8_t)Op::LIT, 3, 0, 0, 0, // 14: DS[0]=3
      (uint8_t)Op::RET              // 19:
  };
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 3);
}

/* ------------------------------------------------------------------------- */
/* Error paths                                                               */
/* ------------------------------------------------------------------------- */
TEST_CASE("error: truncated LIT immediate")
{
  Vm vm{};
  vm_reset(&vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 1, 0, 0 // missing one byte
  };
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == static_cast<int>(Err::TruncatedLiteral));
  CHECK(vm.sp == vm.DS + 0);
}

/* ------------------------------------------------------------------------- */
/* Simple loop                                                               */
/* ------------------------------------------------------------------------- */
TEST_CASE("Simple loop with JNZ")
{
  Vm vm{};
  vm_reset(&vm);

  // Program: count down from 5 to 0 using a loop.
  uint8_t code[32];
  int k = 0;
  emit8(code, &k, (uint8_t)Op::LIT); // Push 5
  emit32(code, &k, 5);
  int loop_start = k;
  emit8(code, &k, (uint8_t)Op::LIT); // Push 1
  emit32(code, &k, 1);
  emit8(code, &k, (uint8_t)Op::SUB); // n = n - 1
  emit8(code, &k, (uint8_t)Op::DUP); // duplicate for JNZ test
  emit8(code, &k, (uint8_t)Op::JNZ); // if not zero, jump back
  emit16(code, &k, (int16_t)(loop_start - (k + 2)));
  emit8(code, &k, (uint8_t)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.DS[0] == 0);      // final value expected to be 0
  CHECK(vm.sp == vm.DS + 1); // single value remains
}

/* ------------------------------------------------------------------------- */
/* Memory operations (LOAD/STORE)                                            */
/* ------------------------------------------------------------------------- */
TEST_CASE("LOAD/STORE roundtrip 32-bit")
{
  alignas(4) uint8_t ram[64] = {};
  VmConfig cfg{};
  cfg.mem = ram;
  cfg.mem_size = sizeof(ram);

  Vm *vm = vm_create(&cfg);
  vm_reset(vm);

  // bc: LIT 0x12345678; LIT 0x10; STORE; LIT 0x10; LOAD; RET
  uint8_t bc[64];
  int k = 0;
  emit8(bc, &k, (uint8_t)Op::LIT);
  emit32(bc, &k, 0x12345678);
  emit8(bc, &k, (uint8_t)Op::LIT);
  emit32(bc, &k, 0x10);
  emit8(bc, &k, (uint8_t)Op::STORE);
  emit8(bc, &k, (uint8_t)Op::LIT);
  emit32(bc, &k, 0x10);
  emit8(bc, &k, (uint8_t)Op::LOAD);
  emit8(bc, &k, (uint8_t)Op::RET);

  REQUIRE(vm_exec_raw(vm, bc, k) == (v4_err)Err::OK);

  CHECK(vm->sp > vm->DS);
  CHECK(*(vm->sp - 1) == 0x12345678);

  vm_destroy(vm);
}

TEST_CASE("LOAD/STORE out-of-bounds is rejected")
{
  alignas(4) uint8_t ram[16] = {};
  VmConfig cfg{};
  cfg.mem = ram;
  cfg.mem_size = sizeof(ram);

  Vm *vm = vm_create(&cfg);
  vm_reset(vm);

  const int32_t bad_addr = (int32_t)sizeof(ram) - 3;

  uint8_t bc[64];
  int k = 0;
  emit8(bc, &k, (uint8_t)Op::LIT);
  emit32(bc, &k, 0xDEADBEEF);
  emit8(bc, &k, (uint8_t)Op::LIT);
  emit32(bc, &k, bad_addr);
  emit8(bc, &k, (uint8_t)Op::STORE);
  emit8(bc, &k, (uint8_t)Op::RET);

  v4_err e = vm_exec_raw(vm, bc, k);
  CHECK(e != (v4_err)Err::OK);

  vm_destroy(vm);
}

TEST_CASE("LOAD/STORE unaligned is rejected (addr % 4 != 0)")
{
  alignas(4) uint8_t ram[32] = {};
  VmConfig cfg{};
  cfg.mem = ram;
  cfg.mem_size = sizeof(ram);

  Vm *vm = vm_create(&cfg);
  vm_reset(vm);

  const int32_t unaligned = 0x02;

  uint8_t bc[64];
  int k = 0;
  // STORE: LIT 0x01020304; LIT 0x02; STORE; RET
  emit8(bc, &k, (uint8_t)Op::LIT);
  emit32(bc, &k, 0x01020304);
  emit8(bc, &k, (uint8_t)Op::LIT);
  emit32(bc, &k, unaligned);
  emit8(bc, &k, (uint8_t)Op::STORE);
  emit8(bc, &k, (uint8_t)Op::RET);

  v4_err e1 = vm_exec_raw(vm, bc, k);
  CHECK(e1 != (v4_err)Err::OK);

  k = 0;
  emit8(bc, &k, (uint8_t)Op::LIT);
  emit32(bc, &k, unaligned);
  emit8(bc, &k, (uint8_t)Op::LOAD);
  emit8(bc, &k, (uint8_t)Op::RET);

  v4_err e2 = vm_exec_raw(vm, bc, k);
  CHECK(e2 != (v4_err)Err::OK);

  vm_destroy(vm);
}