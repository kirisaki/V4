#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstring>

#include "doctest.h"
#include "v4/internal/vm.h"
#include "v4/opcodes.hpp"
#include "v4/vm_api.h"

using Op = v4::Op;

// Helper to emit bytecode
static void emit8(v4_u8* buf, int* pos, v4_u8 byte)
{
  buf[(*pos)++] = byte;
}

static void emit32(v4_u8* buf, int* pos, v4_i32 val)
{
  buf[(*pos)++] = (v4_u8)(val & 0xFF);
  buf[(*pos)++] = (v4_u8)((val >> 8) & 0xFF);
  buf[(*pos)++] = (v4_u8)((val >> 16) & 0xFF);
  buf[(*pos)++] = (v4_u8)((val >> 24) & 0xFF);
}

TEST_CASE("Basic >R (TOR) operation")
{
  Vm vm{};
  vm_reset(&vm);

  // Bytecode: LIT 42; >R; RET
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 42);
  emit8(code, &k, (v4_u8)Op::TOR);
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  // Data stack should be empty
  CHECK(vm.sp == vm.DS);

  // Return stack should have one value
  CHECK(vm.rp == vm.RS + 1);
  CHECK(vm.RS[0] == 42);
}

TEST_CASE("Basic R> (FROMR) operation")
{
  Vm vm{};
  vm_reset(&vm);

  // Bytecode: LIT 99; >R; R>; RET
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 99);
  emit8(code, &k, (v4_u8)Op::TOR);
  emit8(code, &k, (v4_u8)Op::FROMR);
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  // Data stack should have one value
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 99);

  // Return stack should be empty
  CHECK(vm.rp == vm.RS);
}

TEST_CASE("Basic R@ (RFETCH) operation")
{
  Vm vm{};
  vm_reset(&vm);

  // Bytecode: LIT 123; >R; R@; RET
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 123);
  emit8(code, &k, (v4_u8)Op::TOR);
  emit8(code, &k, (v4_u8)Op::RFETCH);
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  // Data stack should have one value (copied from return stack)
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 123);

  // Return stack should still have the value
  CHECK(vm.rp == vm.RS + 1);
  CHECK(vm.RS[0] == 123);
}

TEST_CASE("Multiple values on return stack")
{
  Vm vm{};
  vm_reset(&vm);

  // Bytecode: LIT 10; >R; LIT 20; >R; LIT 30; >R; R>; R>; R>; RET
  // Should result in: 30, 20, 10 on data stack (reverse order)
  v4_u8 code[64];
  int k = 0;

  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 10);
  emit8(code, &k, (v4_u8)Op::TOR);

  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 20);
  emit8(code, &k, (v4_u8)Op::TOR);

  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 30);
  emit8(code, &k, (v4_u8)Op::TOR);

  emit8(code, &k, (v4_u8)Op::FROMR);
  emit8(code, &k, (v4_u8)Op::FROMR);
  emit8(code, &k, (v4_u8)Op::FROMR);
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  // Data stack should have three values in reverse order
  CHECK(vm.sp == vm.DS + 3);
  CHECK(vm.DS[0] == 30);  // First popped
  CHECK(vm.DS[1] == 20);  // Second popped
  CHECK(vm.DS[2] == 10);  // Third popped

  // Return stack should be empty
  CHECK(vm.rp == vm.RS);
}

TEST_CASE("R@ doesn't remove from return stack")
{
  Vm vm{};
  vm_reset(&vm);

  // Bytecode: LIT 77; >R; R@; R@; R@; RET
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 77);
  emit8(code, &k, (v4_u8)Op::TOR);
  emit8(code, &k, (v4_u8)Op::RFETCH);
  emit8(code, &k, (v4_u8)Op::RFETCH);
  emit8(code, &k, (v4_u8)Op::RFETCH);
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  // Data stack should have three copies
  CHECK(vm.sp == vm.DS + 3);
  CHECK(vm.DS[0] == 77);
  CHECK(vm.DS[1] == 77);
  CHECK(vm.DS[2] == 77);

  // Return stack should still have the original value
  CHECK(vm.rp == vm.RS + 1);
  CHECK(vm.RS[0] == 77);
}

TEST_CASE("Practical example: temporary storage")
{
  Vm vm{};
  vm_reset(&vm);

  // Save value, do calculation, restore: 5 >R 10 20 + R> +
  // Result should be 5 + 30 = 35
  v4_u8 code[64];
  int k = 0;

  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 5);
  emit8(code, &k, (v4_u8)Op::TOR);  // Save 5

  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 10);
  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 20);
  emit8(code, &k, (v4_u8)Op::ADD);  // 10 + 20 = 30

  emit8(code, &k, (v4_u8)Op::FROMR);  // Restore 5
  emit8(code, &k, (v4_u8)Op::ADD);    // 30 + 5 = 35
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 35);
  CHECK(vm.rp == vm.RS);
}

TEST_CASE("Return stack overflow detection")
{
  Vm vm{};
  vm_reset(&vm);

  // Try to push 65 values (RS size is 64)
  v4_u8 code[1024];
  int k = 0;

  for (int i = 0; i < 65; i++)
  {
    emit8(code, &k, (v4_u8)Op::LIT);
    emit32(code, &k, i);
    emit8(code, &k, (v4_u8)Op::TOR);
  }
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  // Should return stack overflow error
  CHECK(rc != 0);
}

TEST_CASE("Return stack underflow detection")
{
  Vm vm{};
  vm_reset(&vm);

  // Try to pop from empty return stack
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::FROMR);
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  // Should return stack underflow error
  CHECK(rc != 0);
}

TEST_CASE("R@ on empty return stack")
{
  Vm vm{};
  vm_reset(&vm);

  // Try to fetch from empty return stack
  v4_u8 code[32];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::RFETCH);
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  // Should return stack underflow error
  CHECK(rc != 0);
}

TEST_CASE("Complex stack manipulation")
{
  Vm vm{};
  vm_reset(&vm);

  // ( 1 2 3 -- 1 3 2 ) using return stack
  // Bytecode: LIT 1; LIT 2; LIT 3; >R; SWAP; R>; RET
  v4_u8 code[64];
  int k = 0;

  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 1);
  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 2);
  emit8(code, &k, (v4_u8)Op::LIT);
  emit32(code, &k, 3);
  emit8(code, &k, (v4_u8)Op::TOR);    // Save 3
  emit8(code, &k, (v4_u8)Op::SWAP);   // Swap 1 and 2
  emit8(code, &k, (v4_u8)Op::FROMR);  // Restore 3
  emit8(code, &k, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, code, k);
  CHECK(rc == 0);

  CHECK(vm.sp == vm.DS + 3);
  CHECK(vm.DS[0] == 2);
  CHECK(vm.DS[1] == 1);
  CHECK(vm.DS[2] == 3);
}