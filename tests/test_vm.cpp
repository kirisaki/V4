#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <v4/vm_api.h>
#include <v4/opcodes.hpp>
#include <v4/errors.hpp>

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

TEST_CASE("vm version")
{
  CHECK(v4_vm_version() == 0);
}

TEST_CASE("basic arithmetic (LIT/ADD/RET)")
{
  Vm vm;
  vm_reset(vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::LIT, 20, 0, 0, 0,
      (uint8_t)Op::ADD,
      (uint8_t)Op::RET};
  int rc = vm_exec(vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 30);
}

TEST_CASE("subtraction (LIT/SUB/RET)")
{
  Vm vm;
  vm_reset(vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 20, 0, 0, 0,
      (uint8_t)Op::LIT, 10, 0, 0, 0,
      (uint8_t)Op::SUB,
      (uint8_t)Op::RET};
  int rc = vm_exec(vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 10);
}

TEST_CASE("basic stack ops (LIT/SWAP/DUP/OVER/DROP/RET)")
{
  Vm vm;
  vm_reset(vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 1, 0, 0, 0,
      (uint8_t)Op::LIT, 2, 0, 0, 0,
      (uint8_t)Op::SWAP,
      (uint8_t)Op::DUP,
      (uint8_t)Op::OVER,
      (uint8_t)Op::DROP,
      (uint8_t)Op::RET};
  int rc = vm_exec(vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 3);
  CHECK(vm.DS[0] == 2);
  CHECK(vm.DS[1] == 1);
}

TEST_CASE("unconditional branch (JMP)")
{
  Vm vm;
  vm_reset(vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 1, 0, 0, 0, // 0: DS[0]=1
      (uint8_t)Op::JMP, 5, 0,       // 5: jump to 12
      (uint8_t)Op::LIT, 2, 0, 0, 0, // 7: DS[0]=2 (skipped)
      (uint8_t)Op::RET};            // 12:
  int rc = vm_exec(vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 1);
}

TEST_CASE("conditional branch (JZ)")
{
  Vm vm;
  vm_reset(vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 0, 0, 0, 0, // 0: DS[0]=0
      (uint8_t)Op::JZ, 8, 0,        // 5: jump to 14 if DS[0]==0
      (uint8_t)Op::LIT, 1, 0, 0, 0, // 7: DS[0]=1 (skipped)
      (uint8_t)Op::JMP, 6, 0,       // 12: jump to 20
      (uint8_t)Op::LIT, 2, 0, 0, 0, // 14: DS[0]=2
      (uint8_t)Op::RET};            // 19:
  int rc = vm_exec(vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 2);
}

TEST_CASE("conditional branch (JNZ)")
{
  Vm vm;
  vm_reset(vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 1, 0, 0, 0, // 0: DS[0]=1
      (uint8_t)Op::JNZ, 8, 0,       // 5: jump to 14 if DS[0]!=0
      (uint8_t)Op::LIT, 2, 0, 0, 0, // 7: DS[0]=2 (skipped)
      (uint8_t)Op::JMP, 6, 0,       // 12: jump to 20
      (uint8_t)Op::LIT, 3, 0, 0, 0, // 14: DS[0]=3
      (uint8_t)Op::RET};            // 19:
  int rc = vm_exec(vm, code, sizeof(code));
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 3);
}

TEST_CASE("error: truncated LIT immediate")
{
  Vm vm;
  vm_reset(vm);
  uint8_t code[] = {
      (uint8_t)Op::LIT, 1, 0, 0}; // missing one byte
  int rc = vm_exec(vm, code, sizeof(code));
  CHECK(rc == static_cast<int>(Err::TruncatedLiteral));
  CHECK(vm.sp == vm.DS + 0);
}

TEST_CASE("Simple loop with JNZ")
{
  Vm vm;
  vm_reset(vm);
  // This program counts down from 5 to 1 using a loop.
  uint8_t code[32];
  int k = 0;
  emit8(code, &k, (uint8_t)Op::LIT); // Push initial value 5
  emit32(code, &k, 5);
  int loop_start = k;
  emit8(code, &k, (uint8_t)Op::LIT); // Push 1
  emit32(code, &k, 1);
  emit8(code, &k, (uint8_t)Op::SUB); // Subtract
  emit8(code, &k, (uint8_t)Op::DUP); // Duplicate result for JNZ check
  emit8(code, &k, (uint8_t)Op::JNZ); // If not zero, jump back to loop_start
  emit16(code, &k, (int16_t)(loop_start - (k + 2)));
  emit8(code, &k, (uint8_t)Op::RET); // Return
  int rc = vm_exec(vm, code, k);
  CHECK(rc == 0);
  CHECK(vm.DS[0] == 0);      // Final value should be 5
  CHECK(vm.sp == vm.DS + 1); // Stack should be empty
}