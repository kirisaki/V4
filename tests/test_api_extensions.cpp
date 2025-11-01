#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "v4/internal/vm.h"
#include "v4/vm_api.h"

// Test helper to reset VM
static void reset_vm(Vm* vm)
{
  vm_reset(vm);
}

// ============================================================================
// vm_find_word() tests
// ============================================================================

TEST_CASE("vm_find_word - basic search")
{
  Vm vm{};
  reset_vm(&vm);

  // Register words
  v4_u8 code1[4] = {0x51};  // RET
  v4_u8 code2[4] = {0x51};

  vm_register_word(&vm, "SQUARE", code1, 1);
  vm_register_word(&vm, "DOUBLE", code2, 1);

  // Search
  CHECK(vm_find_word(&vm, "SQUARE") == 0);
  CHECK(vm_find_word(&vm, "DOUBLE") == 1);
  CHECK(vm_find_word(&vm, "UNKNOWN") < 0);
}

TEST_CASE("vm_find_word - NULL handling")
{
  Vm vm{};
  reset_vm(&vm);

  CHECK(vm_find_word(nullptr, "test") < 0);
  CHECK(vm_find_word(&vm, nullptr) < 0);
}

TEST_CASE("vm_find_word - case sensitive")
{
  Vm vm{};
  reset_vm(&vm);

  v4_u8 code[4] = {0x51};
  vm_register_word(&vm, "test", code, 1);

  CHECK(vm_find_word(&vm, "test") == 0);
  CHECK(vm_find_word(&vm, "TEST") < 0);  // Case sensitive
  CHECK(vm_find_word(&vm, "Test") < 0);
}

TEST_CASE("vm_find_word - empty dictionary")
{
  Vm vm{};
  reset_vm(&vm);

  CHECK(vm_find_word(&vm, "ANYTHING") < 0);
}

TEST_CASE("vm_find_word - anonymous words")
{
  Vm vm{};
  reset_vm(&vm);

  v4_u8 code[4] = {0x51};
  vm_register_word(&vm, nullptr, code, 1);  // Anonymous word
  vm_register_word(&vm, "NAMED", code, 1);

  // Anonymous words cannot be found by name
  CHECK(vm_find_word(&vm, "NAMED") == 1);
  CHECK(vm_find_word(&vm, "") < 0);
}

TEST_CASE("vm_find_word - multiple words with similar names")
{
  Vm vm{};
  reset_vm(&vm);

  v4_u8 code[4] = {0x51};
  vm_register_word(&vm, "ADD", code, 1);
  vm_register_word(&vm, "ADDR", code, 1);
  vm_register_word(&vm, "ADD1", code, 1);

  CHECK(vm_find_word(&vm, "ADD") == 0);
  CHECK(vm_find_word(&vm, "ADDR") == 1);
  CHECK(vm_find_word(&vm, "ADD1") == 2);
}
