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
static inline void emit8(v4_u8 *code, int *k, uint8_t v)
{
  code[(*k)++] = v;
}
static inline void emit16(v4_u8 *code, int *k, int16_t v)
{
  code[(*k)++] = (v4_u8)(v & 0xFF);
  code[(*k)++] = (v4_u8)((v >> 8) & 0xFF);
}
static inline void emit32(v4_u8 *code, int *k, v4_i32 v)
{
  code[(*k)++] = (v4_u8)(v & 0xFF);
  code[(*k)++] = (v4_u8)((v >> 8) & 0xFF);
  code[(*k)++] = (v4_u8)((v >> 16) & 0xFF);
  code[(*k)++] = (v4_u8)((v >> 24) & 0xFF);
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
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 1, 0, 0, 0,
      (v4_u8)Op::LIT, 2, 0, 0, 0,
      (v4_u8)Op::SWAP,
      (v4_u8)Op::DUP,
      (v4_u8)Op::OVER,
      (v4_u8)Op::DROP,
      (v4_u8)Op::RET};
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
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::LIT, 20, 0, 0, 0,
      (v4_u8)Op::ADD,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 30);
}

TEST_CASE("subtraction (LIT/SUB/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 20, 0, 0, 0,
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::SUB,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 10);
}

TEST_CASE("multiplication (LIT/MUL/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 6, 0, 0, 0,
      (v4_u8)Op::LIT, 7, 0, 0, 0,
      (v4_u8)Op::MUL,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 42);
}

TEST_CASE("division (LIT/DIV/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 42, 0, 0, 0,
      (v4_u8)Op::LIT, 7, 0, 0, 0,
      (v4_u8)Op::DIV,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 6);
}

TEST_CASE("modulus (LIT/MOD/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 43, 0, 0, 0,
      (v4_u8)Op::LIT, 7, 0, 0, 0,
      (v4_u8)Op::MOD,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 1);
}

TEST_CASE("error: division by zero")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 42, 0, 0, 0,
      (v4_u8)Op::LIT, 0, 0, 0, 0,
      (v4_u8)Op::DIV,
      (v4_u8)Op::RET};
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
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::EQ,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/NE/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::LIT, 20, 0, 0, 0,
      (v4_u8)Op::NE,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/GT/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 20, 0, 0, 0,
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::GT,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/GE/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::GE,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/LT/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::LIT, 20, 0, 0, 0,
      (v4_u8)Op::LT,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("comparison (LIT/LE/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::LIT, 10, 0, 0, 0,
      (v4_u8)Op::LE,
      (v4_u8)Op::RET};
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
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 0b1100, 0, 0, 0,
      (v4_u8)Op::LIT, 0b1010, 0, 0, 0,
      (v4_u8)Op::AND,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 0b1000);
}

TEST_CASE("bitwise OR (LIT/OR/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 0b1100, 0, 0, 0,
      (v4_u8)Op::LIT, 0b1010, 0, 0, 0,
      (v4_u8)Op::OR,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 0b1110);
}

TEST_CASE("bitwise INVERT (LIT/INVERT/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 0b1100, 0, 0, 0,
      (v4_u8)Op::INVERT,
      (v4_u8)Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == ~0b1100);
}

TEST_CASE("bitwise XOR (LIT/XOR/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 0b1100, 0, 0, 0,
      (v4_u8)Op::LIT, 0b1010, 0, 0, 0,
      (v4_u8)Op::XOR,
      (v4_u8)Op::RET};
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
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 1, 0, 0, 0, // 0: DS[0]=1
      (v4_u8)Op::JMP, 5, 0,       // 5: jump to 12
      (v4_u8)Op::LIT, 2, 0, 0, 0, // 7: DS[0]=2 (skipped)
      (v4_u8)Op::RET              // 12:
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
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 0, 0, 0, 0, // 0: DS[0]=0
      (v4_u8)Op::JZ, 8, 0,        // 5: jump to 14 if DS[0]==0
      (v4_u8)Op::LIT, 1, 0, 0, 0, // 7: DS[0]=1 (skipped)
      (v4_u8)Op::JMP, 6, 0,       // 12: jump to 20
      (v4_u8)Op::LIT, 2, 0, 0, 0, // 14: DS[0]=2
      (v4_u8)Op::RET              // 19:
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
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 1, 0, 0, 0, // 0: DS[0]=1
      (v4_u8)Op::JNZ, 8, 0,       // 5: jump to 14 if DS[0]!=0
      (v4_u8)Op::LIT, 2, 0, 0, 0, // 7: DS[0]=2 (skipped)
      (v4_u8)Op::JMP, 6, 0,       // 12: jump to 20
      (v4_u8)Op::LIT, 3, 0, 0, 0, // 14: DS[0]=3
      (v4_u8)Op::RET              // 19:
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
  v4_u8 code[] = {
      (v4_u8)Op::LIT, 1, 0, 0 // missing one byte
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
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::LIT); // Push 5
  emit32(code, &k, 5);
  int loop_start = k;
  emit8(code, &k, (v4_u8)Op::LIT); // Push 1
  emit32(code, &k, 1);
  emit8(code, &k, (v4_u8)Op::SUB); // n = n - 1
  emit8(code, &k, (v4_u8)Op::DUP); // duplicate for JNZ test
  emit8(code, &k, (v4_u8)Op::JNZ); // if not zero, jump back
  emit16(code, &k, (int16_t)(loop_start - (k + 2)));
  emit8(code, &k, (v4_u8)Op::RET);

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
  alignas(4) v4_u8 ram[64] = {};
  VmConfig cfg{};
  cfg.mem = ram;
  cfg.mem_size = sizeof(ram);

  Vm *vm = vm_create(&cfg);
  vm_reset(vm);

  // bc: LIT 0x12345678; LIT 0x10; STORE; LIT 0x10; LOAD; RET
  v4_u8 bc[64];
  int k = 0;
  emit8(bc, &k, (v4_u8)Op::LIT);
  emit32(bc, &k, 0x12345678);
  emit8(bc, &k, (v4_u8)Op::LIT);
  emit32(bc, &k, 0x10);
  emit8(bc, &k, (v4_u8)Op::STORE);
  emit8(bc, &k, (v4_u8)Op::LIT);
  emit32(bc, &k, 0x10);
  emit8(bc, &k, (v4_u8)Op::LOAD);
  emit8(bc, &k, (v4_u8)Op::RET);

  REQUIRE(vm_exec_raw(vm, bc, k) == (v4_err)Err::OK);

  CHECK(vm->sp > vm->DS);
  CHECK(*(vm->sp - 1) == 0x12345678);

  vm_destroy(vm);
}

TEST_CASE("LOAD/STORE out-of-bounds is rejected")
{
  alignas(4) v4_u8 ram[16] = {};
  VmConfig cfg{};
  cfg.mem = ram;
  cfg.mem_size = sizeof(ram);

  Vm *vm = vm_create(&cfg);
  vm_reset(vm);

  const v4_i32 bad_addr = (v4_i32)sizeof(ram) - 3;

  v4_u8 bc[64];
  int k = 0;
  emit8(bc, &k, (v4_u8)Op::LIT);
  emit32(bc, &k, 0xDEADBEEF);
  emit8(bc, &k, (v4_u8)Op::LIT);
  emit32(bc, &k, bad_addr);
  emit8(bc, &k, (v4_u8)Op::STORE);
  emit8(bc, &k, (v4_u8)Op::RET);

  v4_err e = vm_exec_raw(vm, bc, k);
  CHECK(e != (v4_err)Err::OK);

  vm_destroy(vm);
}

TEST_CASE("LOAD/STORE unaligned is rejected (addr % 4 != 0)")
{
  alignas(4) v4_u8 ram[32] = {};
  VmConfig cfg{};
  cfg.mem = ram;
  cfg.mem_size = sizeof(ram);

  Vm *vm = vm_create(&cfg);
  vm_reset(vm);

  const v4_i32 unaligned = 0x02;

  v4_u8 bc[64];
  int k = 0;
  // STORE: LIT 0x01020304; LIT 0x02; STORE; RET
  emit8(bc, &k, (v4_u8)Op::LIT);
  emit32(bc, &k, 0x01020304);
  emit8(bc, &k, (v4_u8)Op::LIT);
  emit32(bc, &k, unaligned);
  emit8(bc, &k, (v4_u8)Op::STORE);
  emit8(bc, &k, (v4_u8)Op::RET);

  v4_err e1 = vm_exec_raw(vm, bc, k);
  CHECK(e1 != (v4_err)Err::OK);

  k = 0;
  emit8(bc, &k, (v4_u8)Op::LIT);
  emit32(bc, &k, unaligned);
  emit8(bc, &k, (v4_u8)Op::LOAD);
  emit8(bc, &k, (v4_u8)Op::RET);

  v4_err e2 = vm_exec_raw(vm, bc, k);
  CHECK(e2 != (v4_err)Err::OK);

  vm_destroy(vm);
}