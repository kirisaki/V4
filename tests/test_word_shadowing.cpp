/**
 * @file test_word_shadowing.cpp
 * @brief Tests for Forth word shadowing semantics
 *
 * Word shadowing is a fundamental Forth feature where newer word definitions
 * with the same name hide (shadow) older definitions. This file tests that
 * the VM correctly implements this behavior through backward search in the
 * word dictionary.
 *
 * @copyright Copyright 2025 Akihito Kirisaki
 * @license Dual-licensed under MIT or Apache-2.0
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstdint>
#include <cstring>

#include "doctest.h"
#include "v4/internal/vm.h"
#include "v4/opcodes.hpp"
#include "v4/vm_api.h"

/* ========================================================================= */
/* Helper Functions                                                          */
/* ========================================================================= */

/**
 * @brief Create a simple bytecode that pushes a value and returns
 */
static void make_lit_ret_bytecode(uint8_t* code, int32_t value)
{
  code[0] = static_cast<uint8_t>(v4::Op::LIT);
  code[1] = value & 0xFF;
  code[2] = (value >> 8) & 0xFF;
  code[3] = (value >> 16) & 0xFF;
  code[4] = (value >> 24) & 0xFF;
  code[5] = static_cast<uint8_t>(v4::Op::RET);
}

/* ========================================================================= */
/* vm_find_word() Tests                                                      */
/* ========================================================================= */

TEST_CASE("vm_find_word: Basic word lookup")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code1[6];
  make_lit_ret_bytecode(code1, 10);

  const int idx = vm_register_word(&vm, "TEST", code1, 6);
  REQUIRE(idx >= 0);

  const int found = vm_find_word(&vm, "TEST");
  CHECK(found == idx);
}

TEST_CASE("vm_find_word: Not found returns -1")
{
  Vm vm{};
  vm_reset(&vm);

  const int found = vm_find_word(&vm, "NONEXISTENT");
  CHECK(found == -1);
}

TEST_CASE("vm_find_word: Case sensitive search")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code1[6];
  make_lit_ret_bytecode(code1, 10);

  const int idx = vm_register_word(&vm, "MyWord", code1, 6);
  REQUIRE(idx >= 0);

  // Should find with exact case
  CHECK(vm_find_word(&vm, "MyWord") == idx);

  // Should NOT find with different case
  CHECK(vm_find_word(&vm, "MYWORD") == -1);
  CHECK(vm_find_word(&vm, "myword") == -1);
}

TEST_CASE("vm_find_word: Null checks")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code1[6];
  make_lit_ret_bytecode(code1, 10);
  vm_register_word(&vm, "TEST", code1, 6);

  CHECK(vm_find_word(nullptr, "TEST") == -1);
  CHECK(vm_find_word(&vm, nullptr) == -1);
}

/* ========================================================================= */
/* Word Shadowing Tests                                                      */
/* ========================================================================= */

TEST_CASE("Word shadowing: Newer definition shadows older")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code1[6], code2[6];
  make_lit_ret_bytecode(code1, 10);   // First definition: returns 10
  make_lit_ret_bytecode(code2, 20);   // Second definition: returns 20

  const int idx1 = vm_register_word(&vm, "FOO", code1, 6);
  const int idx2 = vm_register_word(&vm, "FOO", code2, 6);

  REQUIRE(idx1 >= 0);
  REQUIRE(idx2 >= 0);
  REQUIRE(idx2 != idx1);  // Different indices

  // vm_find_word should return the NEWEST definition (idx2)
  const int found = vm_find_word(&vm, "FOO");
  CHECK(found == idx2);
}

TEST_CASE("Word shadowing: Multiple redefinitions")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code1[6], code2[6], code3[6];
  make_lit_ret_bytecode(code1, 10);
  make_lit_ret_bytecode(code2, 20);
  make_lit_ret_bytecode(code3, 30);

  const int idx1 = vm_register_word(&vm, "TEST", code1, 6);
  const int idx2 = vm_register_word(&vm, "TEST", code2, 6);
  const int idx3 = vm_register_word(&vm, "TEST", code3, 6);

  REQUIRE(idx1 >= 0);
  REQUIRE(idx2 >= 0);
  REQUIRE(idx3 >= 0);

  // Should always find the most recent definition
  CHECK(vm_find_word(&vm, "TEST") == idx3);
}

TEST_CASE("Word shadowing: Execution uses newest definition")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code1[6], code2[6];
  make_lit_ret_bytecode(code1, 100);
  make_lit_ret_bytecode(code2, 200);

  // Register first definition
  const int idx1 = vm_register_word(&vm, "VALUE", code1, 6);
  REQUIRE(idx1 >= 0);

  // Create main bytecode that calls VALUE
  uint8_t main_code1[] = {
      static_cast<uint8_t>(v4::Op::CALL),
      static_cast<uint8_t>(idx1 & 0xFF),
      static_cast<uint8_t>((idx1 >> 8) & 0xFF),
      static_cast<uint8_t>(v4::Op::RET)};

  // Execute: should push 100
  const int rc1 = vm_exec_raw(&vm, main_code1, sizeof(main_code1));
  CHECK(rc1 == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 100);

  // Reset
  vm_reset(&vm);

  // Register first again, then second definition (shadows first)
  vm_register_word(&vm, "VALUE", code1, 6);
  const int idx2 = vm_register_word(&vm, "VALUE", code2, 6);
  REQUIRE(idx2 >= 0);

  // Create main bytecode that calls newest VALUE
  uint8_t main_code2[] = {
      static_cast<uint8_t>(v4::Op::CALL),
      static_cast<uint8_t>(idx2 & 0xFF),
      static_cast<uint8_t>((idx2 >> 8) & 0xFF),
      static_cast<uint8_t>(v4::Op::RET)};

  // Execute: should push 200 (from new definition)
  const int rc2 = vm_exec_raw(&vm, main_code2, sizeof(main_code2));
  CHECK(rc2 == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 200);
}

TEST_CASE("Word shadowing: Different words don't interfere")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code1[6], code2[6], code3[6];
  make_lit_ret_bytecode(code1, 10);
  make_lit_ret_bytecode(code2, 20);
  make_lit_ret_bytecode(code3, 30);

  const int idx_foo1 = vm_register_word(&vm, "FOO", code1, 6);
  const int idx_bar = vm_register_word(&vm, "BAR", code2, 6);
  const int idx_foo2 = vm_register_word(&vm, "FOO", code3, 6);

  // FOO should resolve to newest definition
  CHECK(vm_find_word(&vm, "FOO") == idx_foo2);

  // BAR should still be found correctly
  CHECK(vm_find_word(&vm, "BAR") == idx_bar);
}

TEST_CASE("Word shadowing: Anonymous words don't shadow")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code1[6], code2[6], code3[6];
  make_lit_ret_bytecode(code1, 10);
  make_lit_ret_bytecode(code2, 20);
  make_lit_ret_bytecode(code3, 30);

  const int idx_anon1 = vm_register_word(&vm, nullptr, code1, 6);
  const int idx_named = vm_register_word(&vm, "FOO", code2, 6);
  const int idx_anon2 = vm_register_word(&vm, nullptr, code3, 6);

  // Named word should be found
  CHECK(vm_find_word(&vm, "FOO") == idx_named);

  // All word indices should be valid
  REQUIRE(idx_anon1 >= 0);
  REQUIRE(idx_named >= 0);
  REQUIRE(idx_anon2 >= 0);
}

/* ========================================================================= */
/* Edge Cases                                                                */
/* ========================================================================= */

TEST_CASE("Word shadowing: Many redefinitions")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code[6];
  make_lit_ret_bytecode(code, 42);

  // Register 10 words with the same name
  int last_idx = -1;
  for (int i = 0; i < 10; i++)
  {
    const int idx = vm_register_word(&vm, "TEST", code, 6);
    REQUIRE(idx >= 0);
    last_idx = idx;
  }

  // Should find the newest TEST
  CHECK(vm_find_word(&vm, "TEST") == last_idx);
}

TEST_CASE("Word shadowing: Empty name vs NULL name")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code1[6], code2[6];
  make_lit_ret_bytecode(code1, 10);
  make_lit_ret_bytecode(code2, 20);

  // Register word with empty string name
  const int idx_empty = vm_register_word(&vm, "", code1, 6);

  // Register word with NULL name (anonymous)
  const int idx_null = vm_register_word(&vm, nullptr, code2, 6);

  REQUIRE(idx_empty >= 0);
  REQUIRE(idx_null >= 0);

  // Empty string should be findable
  CHECK(vm_find_word(&vm, "") == idx_empty);

  // NULL name should not be findable
  CHECK(vm_find_word(&vm, nullptr) == -1);
}

TEST_CASE("Word shadowing: Interleaved definitions")
{
  Vm vm{};
  vm_reset(&vm);

  uint8_t code[6];
  make_lit_ret_bytecode(code, 42);

  // Register: FOO, BAR, FOO, BAZ, BAR, FOO
  const int idx_foo1 = vm_register_word(&vm, "FOO", code, 6);
  const int idx_bar1 = vm_register_word(&vm, "BAR", code, 6);
  const int idx_foo2 = vm_register_word(&vm, "FOO", code, 6);
  const int idx_baz = vm_register_word(&vm, "BAZ", code, 6);
  const int idx_bar2 = vm_register_word(&vm, "BAR", code, 6);
  const int idx_foo3 = vm_register_word(&vm, "FOO", code, 6);

  // Check that newest definitions are found
  CHECK(vm_find_word(&vm, "FOO") == idx_foo3);
  CHECK(vm_find_word(&vm, "BAR") == idx_bar2);
  CHECK(vm_find_word(&vm, "BAZ") == idx_baz);

  // Verify all indices are different
  REQUIRE(idx_foo1 != idx_foo2);
  REQUIRE(idx_foo2 != idx_foo3);
  REQUIRE(idx_bar1 != idx_bar2);
}
