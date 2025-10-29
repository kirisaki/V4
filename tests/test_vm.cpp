#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstdint>

#include "doctest.h"
#include "v4/errors.hpp"
#include "v4/internal/vm.h"
#include "v4/opcodes.hpp"
#include "v4/vm_api.h"

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
  v4_u8 code[] = {(v4_u8)v4::Op::LIT,
                  1,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::LIT,
                  2,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::SWAP,
                  (v4_u8)v4::Op::DUP,
                  (v4_u8)v4::Op::OVER,
                  (v4_u8)v4::Op::DROP,
                  (v4_u8)v4::Op::RET};
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
  v4_u8 code[] = {(v4_u8)v4::Op::LIT,
                  10,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::LIT,
                  20,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::ADD,
                  (v4_u8)v4::Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 30);
}

TEST_CASE("subtraction (LIT/SUB/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {(v4_u8)v4::Op::LIT,
                  20,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::LIT,
                  10,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::SUB,
                  (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 6, 0, 0, 0, (v4_u8)v4::Op::LIT, 7, 0, 0, 0, (v4_u8)v4::Op::MUL,
      (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 42, 0, 0, 0, (v4_u8)v4::Op::LIT, 7, 0, 0, 0, (v4_u8)v4::Op::DIV,
      (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 43, 0, 0, 0, (v4_u8)v4::Op::LIT, 7, 0, 0, 0, (v4_u8)v4::Op::MOD,
      (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 42, 0, 0, 0, (v4_u8)v4::Op::LIT, 0, 0, 0, 0, (v4_u8)v4::Op::DIV,
      (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 10, 0, 0, 0, (v4_u8)v4::Op::LIT, 10, 0, 0, 0, (v4_u8)v4::Op::EQ,
      (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 10, 0, 0, 0, (v4_u8)v4::Op::LIT, 20, 0, 0, 0, (v4_u8)v4::Op::NE,
      (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 20, 0, 0, 0, (v4_u8)v4::Op::LIT, 10, 0, 0, 0, (v4_u8)v4::Op::GT,
      (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 10, 0, 0, 0, (v4_u8)v4::Op::LIT, 10, 0, 0, 0, (v4_u8)v4::Op::GE,
      (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 10, 0, 0, 0, (v4_u8)v4::Op::LIT, 20, 0, 0, 0, (v4_u8)v4::Op::LT,
      (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 10, 0, 0, 0, (v4_u8)v4::Op::LIT, 10, 0, 0, 0, (v4_u8)v4::Op::LE,
      (v4_u8)v4::Op::RET};
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
  v4_u8 code[] = {(v4_u8)v4::Op::LIT,
                  0b1100,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::LIT,
                  0b1010,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::AND,
                  (v4_u8)v4::Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 0b1000);
}

TEST_CASE("bitwise OR (LIT/OR/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {(v4_u8)v4::Op::LIT,
                  0b1100,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::LIT,
                  0b1010,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::OR,
                  (v4_u8)v4::Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 0b1110);
}

TEST_CASE("bitwise INVERT (LIT/INVERT/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {(v4_u8)v4::Op::LIT, 0b1100, 0, 0, 0, (v4_u8)v4::Op::INVERT,
                  (v4_u8)v4::Op::RET};
  int rc = vm_exec_raw(&vm, code, (int)sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == ~0b1100);
}

TEST_CASE("bitwise XOR (LIT/XOR/RET)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {(v4_u8)v4::Op::LIT,
                  0b1100,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::LIT,
                  0b1010,
                  0,
                  0,
                  0,
                  (v4_u8)v4::Op::XOR,
                  (v4_u8)v4::Op::RET};
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
      (v4_u8)v4::Op::LIT, 1, 0, 0, 0,  // 0: DS[0]=1
      (v4_u8)v4::Op::JMP, 5, 0,        // 5: jump to 12
      (v4_u8)v4::Op::LIT, 2, 0, 0, 0,  // 7: DS[0]=2 (skipped)
      (v4_u8)v4::Op::RET               // 12:
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
      (v4_u8)v4::Op::LIT, 0, 0, 0, 0,  // 0: DS[0]=0
      (v4_u8)v4::Op::JZ,  8, 0,        // 5: jump to 14 if DS[0]==0
      (v4_u8)v4::Op::LIT, 1, 0, 0, 0,  // 7: DS[0]=1 (skipped)
      (v4_u8)v4::Op::JMP, 6, 0,        // 12: jump to 20
      (v4_u8)v4::Op::LIT, 2, 0, 0, 0,  // 14: DS[0]=2
      (v4_u8)v4::Op::RET               // 19:
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
      (v4_u8)v4::Op::LIT, 1, 0, 0, 0,  // 0: DS[0]=1
      (v4_u8)v4::Op::JNZ, 8, 0,        // 5: jump to 14 if DS[0]!=0
      (v4_u8)v4::Op::LIT, 2, 0, 0, 0,  // 7: DS[0]=2 (skipped)
      (v4_u8)v4::Op::JMP, 6, 0,        // 12: jump to 20
      (v4_u8)v4::Op::LIT, 3, 0, 0, 0,  // 14: DS[0]=3
      (v4_u8)v4::Op::RET               // 19:
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
      (v4_u8)v4::Op::LIT, 1, 0, 0  // missing one byte
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
  emit8(code, &k, (v4_u8)v4::Op::LIT);  // Push 5
  emit32(code, &k, 5);
  int loop_start = k;
  emit8(code, &k, (v4_u8)v4::Op::LIT);  // Push 1
  emit32(code, &k, 1);
  emit8(code, &k, (v4_u8)v4::Op::SUB);  // n = n - 1
  emit8(code, &k, (v4_u8)v4::Op::DUP);  // duplicate for JNZ test
  emit8(code, &k, (v4_u8)v4::Op::JNZ);  // if not zero, jump back
  emit16(code, &k, (int16_t)(loop_start - (k + 2)));
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.DS[0] == 0);       // final value expected to be 0
  CHECK(vm.sp == vm.DS + 1);  // single value remains
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
  emit8(bc, &k, (v4_u8)v4::Op::LIT);
  emit32(bc, &k, 0x12345678);
  emit8(bc, &k, (v4_u8)v4::Op::LIT);
  emit32(bc, &k, 0x10);
  emit8(bc, &k, (v4_u8)v4::Op::STORE);
  emit8(bc, &k, (v4_u8)v4::Op::LIT);
  emit32(bc, &k, 0x10);
  emit8(bc, &k, (v4_u8)v4::Op::LOAD);
  emit8(bc, &k, (v4_u8)v4::Op::RET);

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
  emit8(bc, &k, (v4_u8)v4::Op::LIT);
  emit32(bc, &k, 0xDEADBEEF);
  emit8(bc, &k, (v4_u8)v4::Op::LIT);
  emit32(bc, &k, bad_addr);
  emit8(bc, &k, (v4_u8)v4::Op::STORE);
  emit8(bc, &k, (v4_u8)v4::Op::RET);

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
  emit8(bc, &k, (v4_u8)v4::Op::LIT);
  emit32(bc, &k, 0x01020304);
  emit8(bc, &k, (v4_u8)v4::Op::LIT);
  emit32(bc, &k, unaligned);
  emit8(bc, &k, (v4_u8)v4::Op::STORE);
  emit8(bc, &k, (v4_u8)v4::Op::RET);

  v4_err e1 = vm_exec_raw(vm, bc, k);
  CHECK(e1 != (v4_err)Err::OK);

  k = 0;
  emit8(bc, &k, (v4_u8)v4::Op::LIT);
  emit32(bc, &k, unaligned);
  emit8(bc, &k, (v4_u8)v4::Op::LOAD);
  emit8(bc, &k, (v4_u8)v4::Op::RET);

  v4_err e2 = vm_exec_raw(vm, bc, k);
  CHECK(e2 != (v4_err)Err::OK);

  vm_destroy(vm);
}

/* ------------------------------------------------------------------------- */
/* Stack inspection API tests                                                */
/* ------------------------------------------------------------------------- */
TEST_CASE("vm_ds_depth_public - empty stack")
{
  Vm vm{};
  vm_reset(&vm);

  CHECK(vm_ds_depth_public(&vm) == 0);
}

TEST_CASE("vm_ds_depth_public - after pushing values")
{
  Vm vm{};
  vm_reset(&vm);

  // LIT 10; LIT 20; LIT 30; RET
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 10);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 20);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 30);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm_ds_depth_public(&vm) == 3);
}

TEST_CASE("vm_ds_depth_public - NULL vm")
{
  CHECK(vm_ds_depth_public(nullptr) == 0);
}

TEST_CASE("vm_ds_peek_public - peek at stack values")
{
  Vm vm{};
  vm_reset(&vm);

  // LIT 10; LIT 20; LIT 30; RET
  // Stack bottom to top: [10, 20, 30]
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 10);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 20);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 30);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  // Peek from top
  CHECK(vm_ds_peek_public(&vm, 0) == 30);  // Top
  CHECK(vm_ds_peek_public(&vm, 1) == 20);  // Second from top
  CHECK(vm_ds_peek_public(&vm, 2) == 10);  // Third from top (bottom)
}

TEST_CASE("vm_ds_peek_public - out of range returns 0")
{
  Vm vm{};
  vm_reset(&vm);

  // LIT 42; RET
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 42);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm_ds_depth_public(&vm) == 1);

  // Valid peek
  CHECK(vm_ds_peek_public(&vm, 0) == 42);

  // Out of range (negative)
  CHECK(vm_ds_peek_public(&vm, -1) == 0);

  // Out of range (too large)
  CHECK(vm_ds_peek_public(&vm, 1) == 0);
  CHECK(vm_ds_peek_public(&vm, 100) == 0);
}

TEST_CASE("vm_ds_peek_public - empty stack")
{
  Vm vm{};
  vm_reset(&vm);

  CHECK(vm_ds_depth_public(&vm) == 0);
  CHECK(vm_ds_peek_public(&vm, 0) == 0);
}

TEST_CASE("vm_ds_peek_public - NULL vm")
{
  CHECK(vm_ds_peek_public(nullptr, 0) == 0);
}

TEST_CASE("Stack inspection - integration test")
{
  Vm vm{};
  vm_reset(&vm);

  // Simulate Forth "5 3 +"
  // LIT 5; LIT 3; ADD; RET
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 5);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 3);
  emit8(code, &k, (v4_u8)v4::Op::ADD);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  // Verify result: stack depth is 1, top value is 8
  CHECK(vm_ds_depth_public(&vm) == 1);
  CHECK(vm_ds_peek_public(&vm, 0) == 8);
}

TEST_CASE("Stack inspection - after complex operations")
{
  Vm vm{};
  vm_reset(&vm);

  // Simulate: 10 DUP * (10 squared = 100)
  // LIT 10; DUP; MUL; RET
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 10);
  emit8(code, &k, (v4_u8)v4::Op::DUP);
  emit8(code, &k, (v4_u8)v4::Op::MUL);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  CHECK(vm_ds_depth_public(&vm) == 1);
  CHECK(vm_ds_peek_public(&vm, 0) == 100);
}
/* ------------------------------------------------------------------------- */
/* REPL support: Stack manipulation API tests                                */
/* ------------------------------------------------------------------------- */
TEST_CASE("Stack manipulation - vm_ds_push and vm_ds_pop")
{
  Vm vm{};
  vm_reset(&vm);

  // Push values
  CHECK(vm_ds_push(&vm, 10) == 0);
  CHECK(vm_ds_push(&vm, 20) == 0);
  CHECK(vm_ds_push(&vm, 30) == 0);
  CHECK(vm_ds_depth_public(&vm) == 3);

  // Pop values
  v4_i32 val;
  CHECK(vm_ds_pop(&vm, &val) == 0);
  CHECK(val == 30);
  CHECK(vm_ds_pop(&vm, &val) == 0);
  CHECK(val == 20);
  CHECK(vm_ds_depth_public(&vm) == 1);

  // Pop without output pointer
  CHECK(vm_ds_pop(&vm, nullptr) == 0);
  CHECK(vm_ds_depth_public(&vm) == 0);
}

TEST_CASE("Stack manipulation - vm_ds_clear")
{
  Vm vm{};
  vm_reset(&vm);

  vm_ds_push(&vm, 1);
  vm_ds_push(&vm, 2);
  vm_ds_push(&vm, 3);
  CHECK(vm_ds_depth_public(&vm) == 3);

  vm_ds_clear(&vm);
  CHECK(vm_ds_depth_public(&vm) == 0);
}

TEST_CASE("Stack manipulation - error handling")
{
  Vm vm{};
  vm_reset(&vm);

  // Stack underflow
  v4_i32 val;
  CHECK(vm_ds_pop(&vm, &val) != 0);

  // NULL vm
  CHECK(vm_ds_push(nullptr, 42) != 0);
  CHECK(vm_ds_pop(nullptr, &val) != 0);
}

TEST_CASE("Stack snapshot and restore - basic")
{
  Vm vm{};
  vm_reset(&vm);

  // Setup initial stack
  vm_ds_push(&vm, 10);
  vm_ds_push(&vm, 20);
  vm_ds_push(&vm, 30);

  // Create snapshot
  VmStackSnapshot *snap = vm_ds_snapshot(&vm);
  REQUIRE(snap != nullptr);
  REQUIRE(snap->depth == 3);
  REQUIRE(snap->data != nullptr);

  // Modify stack
  vm_ds_clear(&vm);
  vm_ds_push(&vm, 999);
  CHECK(vm_ds_depth_public(&vm) == 1);

  // Restore from snapshot
  CHECK(vm_ds_restore(&vm, snap) == 0);
  CHECK(vm_ds_depth_public(&vm) == 3);
  CHECK(vm_ds_peek_public(&vm, 0) == 30);
  CHECK(vm_ds_peek_public(&vm, 1) == 20);
  CHECK(vm_ds_peek_public(&vm, 2) == 10);

  vm_ds_snapshot_free(snap);
}

TEST_CASE("Stack snapshot - empty stack")
{
  Vm vm{};
  vm_reset(&vm);

  // Snapshot empty stack
  VmStackSnapshot *snap = vm_ds_snapshot(&vm);
  REQUIRE(snap != nullptr);
  REQUIRE(snap->depth == 0);

  // Push values then restore empty snapshot
  vm_ds_push(&vm, 42);
  CHECK(vm_ds_depth_public(&vm) == 1);

  CHECK(vm_ds_restore(&vm, snap) == 0);
  CHECK(vm_ds_depth_public(&vm) == 0);

  vm_ds_snapshot_free(snap);
}

TEST_CASE("Stack snapshot - NULL safety")
{
  // NULL snapshot free (should not crash)
  vm_ds_snapshot_free(nullptr);

  // NULL vm snapshot
  CHECK(vm_ds_snapshot(nullptr) == nullptr);

  // NULL restore
  Vm vm{};
  vm_reset(&vm);
  CHECK(vm_ds_restore(nullptr, nullptr) != 0);
  CHECK(vm_ds_restore(&vm, nullptr) != 0);
}

/* ------------------------------------------------------------------------- */
/* REPL support: Selective reset API tests                                  */
/* ------------------------------------------------------------------------- */
TEST_CASE("Selective reset - vm_reset_dictionary preserves stacks")
{
  Vm vm{};
  vm_reset(&vm);

  // Register words and push values
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::RET);

  vm_register_word(&vm, "WORD1", code, k);
  vm_register_word(&vm, "WORD2", code, k);
  vm_ds_push(&vm, 42);
  vm_ds_push(&vm, 100);

  CHECK(vm.word_count == 2);
  CHECK(vm_ds_depth_public(&vm) == 2);

  // Reset dictionary only
  vm_reset_dictionary(&vm);

  // Stack should be preserved
  CHECK(vm_ds_depth_public(&vm) == 2);
  CHECK(vm_ds_peek_public(&vm, 0) == 100);
  CHECK(vm_ds_peek_public(&vm, 1) == 42);

  // Words should be cleared
  CHECK(vm.word_count == 0);
}

TEST_CASE("Selective reset - vm_reset_stacks preserves dictionary")
{
  Vm vm{};
  vm_reset(&vm);

  // Register words and push values
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::RET);

  vm_register_word(&vm, "WORD1", code, k);
  vm_ds_push(&vm, 42);
  vm_ds_push(&vm, 100);

  CHECK(vm.word_count == 1);
  CHECK(vm_ds_depth_public(&vm) == 2);

  // Reset stacks only
  vm_reset_stacks(&vm);

  // Stacks should be cleared
  CHECK(vm_ds_depth_public(&vm) == 0);

  // Dictionary should be preserved
  CHECK(vm.word_count == 1);

  // Clean up word names to prevent memory leak
  vm_reset_dictionary(&vm);
}

TEST_CASE("Selective reset - vm_reset clears everything")
{
  Vm vm{};
  vm_reset(&vm);

  // Register words and push values
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::RET);

  vm_register_word(&vm, "WORD1", code, k);
  vm_ds_push(&vm, 42);

  // Full reset
  vm_reset(&vm);

  // Everything should be cleared
  CHECK(vm_ds_depth_public(&vm) == 0);
  CHECK(vm.word_count == 0);
}

TEST_CASE("REPL use case - preserve stack across word definition")
{
  Vm vm{};
  vm_reset(&vm);

  // User evaluates: "1 2 +" (result: 3)
  vm_ds_push(&vm, 3);
  CHECK(vm_ds_depth_public(&vm) == 1);

  // User evaluates: "10 20 +" (result: 30)
  vm_ds_push(&vm, 30);
  CHECK(vm_ds_depth_public(&vm) == 2);

  // User defines: ": SQUARE DUP * ;"
  // REPL needs to reset dictionary but preserve stack
  VmStackSnapshot *snap = vm_ds_snapshot(&vm);
  vm_reset_dictionary(&vm);
  vm_ds_restore(&vm, snap);
  vm_ds_snapshot_free(snap);

  // Stack should still have [3, 30]
  CHECK(vm_ds_depth_public(&vm) == 2);
  CHECK(vm_ds_peek_public(&vm, 1) == 3);
  CHECK(vm_ds_peek_public(&vm, 0) == 30);

  // Dictionary is clear for new word
  CHECK(vm.word_count == 0);

  // Now register SQUARE
  v4_u8 square_code[16];
  int k = 0;
  emit8(square_code, &k, (v4_u8)v4::Op::DUP);
  emit8(square_code, &k, (v4_u8)v4::Op::MUL);
  emit8(square_code, &k, (v4_u8)v4::Op::RET);

  vm_register_word(&vm, "SQUARE", square_code, k);
  CHECK(vm.word_count == 1);

  // Execute "5 SQUARE"
  vm_ds_push(&vm, 5);
  v4_u8 call_code[16];
  k = 0;
  emit8(call_code, &k, (v4_u8)v4::Op::CALL);
  emit8(call_code, &k, 0);  // Word index 0
  emit8(call_code, &k, 0);
  emit8(call_code, &k, (v4_u8)v4::Op::RET);

  vm_exec_raw(&vm, call_code, k);

  // Stack should be [3, 30, 25]
  CHECK(vm_ds_depth_public(&vm) == 3);
  CHECK(vm_ds_peek_public(&vm, 2) == 3);
  CHECK(vm_ds_peek_public(&vm, 1) == 30);
  CHECK(vm_ds_peek_public(&vm, 0) == 25);

  // Clean up word names to prevent memory leak
  vm_reset_dictionary(&vm);
}

/* ------------------------------------------------------------------------- */
/* Extended arithmetic operations (Commit 1)                                 */
/* ------------------------------------------------------------------------- */
TEST_CASE("unsigned division (DIVU)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, -10);  // 0xFFFFFFF6 as unsigned = 4294967286
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 2);
  emit8(code, &k, (v4_u8)v4::Op::DIVU);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  // -10 as unsigned / 2 = 2147483643
  CHECK((v4_u32)vm.DS[0] == 2147483643u);
}

TEST_CASE("unsigned modulo (MODU)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, -10);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 3);
  emit8(code, &k, (v4_u8)v4::Op::MODU);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  // -10 as unsigned % 3
  CHECK((v4_u32)vm.DS[0] == (4294967286u % 3));
}

TEST_CASE("increment (INC)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 42);
  emit8(code, &k, (v4_u8)v4::Op::INC);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 43);
}

TEST_CASE("decrement (DEC)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 42);
  emit8(code, &k, (v4_u8)v4::Op::DEC);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 41);
}

TEST_CASE("unsigned less than (LTU)")
{
  Vm vm{};
  vm_reset(&vm);

  // Test -1 < 1 (unsigned: 0xFFFFFFFF < 1 = false)
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, -1);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 1);
  emit8(code, &k, (v4_u8)v4::Op::LTU);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == V4_FALSE);  // 0xFFFFFFFF is not < 1 unsigned

  // Test 1 < 10 (unsigned: true)
  vm_reset(&vm);
  k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 1);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 10);
  emit8(code, &k, (v4_u8)v4::Op::LTU);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("unsigned less than or equal (LEU)")
{
  Vm vm{};
  vm_reset(&vm);

  // Test 5 <= 5 (unsigned: true)
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 5);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 5);
  emit8(code, &k, (v4_u8)v4::Op::LEU);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.DS[0] == V4_TRUE);
}

TEST_CASE("shift left (SHL)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 3);  // value
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 2);  // shift amount
  emit8(code, &k, (v4_u8)v4::Op::SHL);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 12);  // 3 << 2 = 12
}

TEST_CASE("shift right logical (SHR)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, -8);  // 0xFFFFFFF8
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 2);
  emit8(code, &k, (v4_u8)v4::Op::SHR);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  // -8 >> 2 (logical) = 0x3FFFFFFE
  CHECK((v4_u32)vm.DS[0] == 0x3FFFFFFEu);
}

TEST_CASE("shift right arithmetic (SAR)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, -8);
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 2);
  emit8(code, &k, (v4_u8)v4::Op::SAR);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == -2);  // -8 >> 2 (arithmetic) = -2
}

/* ------------------------------------------------------------------------- */
/* Control flow and compact literals (Commit 3)                              */
/* ------------------------------------------------------------------------- */
TEST_CASE("SELECT instruction (ternary operator)")
{
  Vm vm{};
  vm_reset(&vm);

  // Test flag ? 10 : 20 with flag = true
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, V4_TRUE);  // flag
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 20);  // b (false branch)
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 10);  // a (true branch)
  emit8(code, &k, (v4_u8)v4::Op::SELECT);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 10);  // Selected a

  // Test with flag = false
  vm_reset(&vm);
  k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, V4_FALSE);  // flag
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 20);  // b
  emit8(code, &k, (v4_u8)v4::Op::LIT);
  emit32(code, &k, 10);  // a
  emit8(code, &k, (v4_u8)v4::Op::SELECT);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.DS[0] == 20);  // Selected b
}

TEST_CASE("compact literals (LIT0, LIT1, LITN1)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {(v4_u8)v4::Op::LIT0, (v4_u8)v4::Op::LIT1, (v4_u8)v4::Op::LITN1,
                  (v4_u8)v4::Op::RET};

  int rc = vm_exec_raw(&vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 3);
  CHECK(vm.DS[0] == 0);
  CHECK(vm.DS[1] == 1);
  CHECK(vm.DS[2] == -1);
}

TEST_CASE("compact literals (LIT_U8)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {(v4_u8)v4::Op::LIT_U8, 0xFF, (v4_u8)v4::Op::RET};

  int rc = vm_exec_raw(&vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK((v4_u32)vm.DS[0] == 0xFF);  // Unsigned, not sign-extended
}

TEST_CASE("compact literals (LIT_I8)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[] = {(v4_u8)v4::Op::LIT_I8, 0xFF, (v4_u8)v4::Op::RET};

  int rc = vm_exec_raw(&vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == -1);  // Signed, sign-extended
}

TEST_CASE("compact literals (LIT_I16)")
{
  Vm vm{};
  vm_reset(&vm);
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LIT_I16);
  emit16(code, &k, -1000);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == -1000);
}

/* ------------------------------------------------------------------------- */
/* Local variable support (Commit 4)                                         */
/* ------------------------------------------------------------------------- */
TEST_CASE("local variables (LGET/LSET)")
{
  Vm vm{};
  vm_reset(&vm);

  // Setup: Manually create a local frame in return stack
  // Simulate having 3 locals
  vm.fp = vm.RS;  // Set frame pointer
  vm.RS[0] = 100;
  vm.RS[1] = 200;
  vm.RS[2] = 300;
  vm.rp = vm.RS + 3;

  // Test LGET 0
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LGET);
  emit8(code, &k, 0);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 100);

  // Test LSET 1
  vm_reset(&vm);
  vm.fp = vm.RS;
  vm.RS[0] = 100;
  vm.RS[1] = 200;
  vm.rp = vm.RS + 2;

  vm_ds_push(&vm, 999);  // New value
  k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LSET);
  emit8(code, &k, 1);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.RS[1] == 999);
}

TEST_CASE("local variables (LTEE)")
{
  Vm vm{};
  vm_reset(&vm);

  vm.fp = vm.RS;
  vm.RS[0] = 0;
  vm.rp = vm.RS + 1;

  vm_ds_push(&vm, 42);
  v4_u8 code[] = {(v4_u8)v4::Op::LTEE, 0, (v4_u8)v4::Op::RET};

  int rc = vm_exec_raw(&vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.RS[0] == 42);      // Local was set
  CHECK(vm.DS[0] == 42);      // Value still on stack
  CHECK(vm.sp == vm.DS + 1);  // Stack unchanged
}

TEST_CASE("local variables (LGET0/LGET1)")
{
  Vm vm{};
  vm_reset(&vm);

  vm.fp = vm.RS;
  vm.RS[0] = 111;
  vm.RS[1] = 222;
  vm.rp = vm.RS + 2;

  v4_u8 code[] = {(v4_u8)v4::Op::LGET0, (v4_u8)v4::Op::LGET1, (v4_u8)v4::Op::RET};

  int rc = vm_exec_raw(&vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 2);
  CHECK(vm.DS[0] == 111);
  CHECK(vm.DS[1] == 222);
}

TEST_CASE("local variables (LSET0/LSET1)")
{
  Vm vm{};
  vm_reset(&vm);

  vm.fp = vm.RS;
  vm.RS[0] = 0;
  vm.RS[1] = 0;
  vm.rp = vm.RS + 2;

  vm_ds_push(&vm, 777);
  vm_ds_push(&vm, 888);
  v4_u8 code[] = {(v4_u8)v4::Op::LSET1, (v4_u8)v4::Op::LSET0, (v4_u8)v4::Op::RET};

  int rc = vm_exec_raw(&vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.RS[0] == 777);
  CHECK(vm.RS[1] == 888);
}

TEST_CASE("local variables (LINC/LDEC)")
{
  Vm vm{};
  vm_reset(&vm);

  vm.fp = vm.RS;
  vm.RS[0] = 10;
  vm.RS[1] = 20;
  vm.rp = vm.RS + 2;

  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)v4::Op::LINC);
  emit8(code, &k, 0);
  emit8(code, &k, (v4_u8)v4::Op::LDEC);
  emit8(code, &k, 1);
  emit8(code, &k, (v4_u8)v4::Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.RS[0] == 11);
  CHECK(vm.RS[1] == 19);
}
