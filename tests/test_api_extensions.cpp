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

  // Cleanup
  vm_reset_dictionary(&vm);
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

  // Cleanup
  vm_reset_dictionary(&vm);
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

  // Cleanup
  vm_reset_dictionary(&vm);
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

  // Cleanup
  vm_reset_dictionary(&vm);
}

// ============================================================================
// vm_ds_copy_to_array() tests
// ============================================================================

TEST_CASE("vm_ds_copy_to_array - basic copy")
{
  Vm vm{};
  reset_vm(&vm);

  // Push values onto stack
  vm_ds_push(&vm, 10);
  vm_ds_push(&vm, 20);
  vm_ds_push(&vm, 30);

  v4_i32 stack[256];
  int count = vm_ds_copy_to_array(&vm, stack, 256);

  CHECK(count == 3);
  CHECK(stack[0] == 10);  // Bottom
  CHECK(stack[1] == 20);
  CHECK(stack[2] == 30);  // TOP
}

TEST_CASE("vm_ds_copy_to_array - partial copy")
{
  Vm vm{};
  reset_vm(&vm);

  vm_ds_push(&vm, 1);
  vm_ds_push(&vm, 2);
  vm_ds_push(&vm, 3);
  vm_ds_push(&vm, 4);
  vm_ds_push(&vm, 5);

  v4_i32 stack[3];
  int count = vm_ds_copy_to_array(&vm, stack, 3);

  CHECK(count == 3);  // Only max_count elements copied
  CHECK(stack[0] == 1);
  CHECK(stack[1] == 2);
  CHECK(stack[2] == 3);
}

TEST_CASE("vm_ds_copy_to_array - empty stack")
{
  Vm vm{};
  reset_vm(&vm);

  v4_i32 stack[256];
  int count = vm_ds_copy_to_array(&vm, stack, 256);

  CHECK(count == 0);
}

TEST_CASE("vm_ds_copy_to_array - NULL handling")
{
  Vm vm{};
  reset_vm(&vm);

  v4_i32 stack[256];

  CHECK(vm_ds_copy_to_array(nullptr, stack, 256) == 0);
  CHECK(vm_ds_copy_to_array(&vm, nullptr, 256) == 0);
  CHECK(vm_ds_copy_to_array(&vm, stack, 0) == 0);
  CHECK(vm_ds_copy_to_array(&vm, stack, -1) == 0);
}

TEST_CASE("vm_ds_copy_to_array - large stack")
{
  Vm vm{};
  reset_vm(&vm);

  // Push 100 values
  for (int i = 0; i < 100; i++)
  {
    vm_ds_push(&vm, i * 10);
  }

  v4_i32 stack[256];
  int count = vm_ds_copy_to_array(&vm, stack, 256);

  CHECK(count == 100);
  CHECK(stack[0] == 0);     // First pushed
  CHECK(stack[50] == 500);  // Middle
  CHECK(stack[99] == 990);  // Last pushed (TOP)
}

// ============================================================================
// vm_rs_depth_public() tests
// ============================================================================

TEST_CASE("vm_rs_depth_public - basic")
{
  Vm vm{};
  reset_vm(&vm);

  CHECK(vm_rs_depth_public(&vm) == 0);

  // Push values onto return stack (internal operation simulation)
  vm.RS[0] = 100;
  vm.RS[1] = 200;
  vm.rp = vm.RS + 2;

  CHECK(vm_rs_depth_public(&vm) == 2);
}

TEST_CASE("vm_rs_depth_public - NULL handling")
{
  CHECK(vm_rs_depth_public(nullptr) == 0);
}

// ============================================================================
// vm_rs_copy_to_array() tests
// ============================================================================

TEST_CASE("vm_rs_copy_to_array - basic copy")
{
  Vm vm{};
  reset_vm(&vm);

  // Set up return stack values
  vm.RS[0] = 100;
  vm.RS[1] = 200;
  vm.RS[2] = 300;
  vm.rp = vm.RS + 3;

  v4_i32 rs[256];
  int count = vm_rs_copy_to_array(&vm, rs, 256);

  CHECK(count == 3);
  CHECK(rs[0] == 100);
  CHECK(rs[1] == 200);
  CHECK(rs[2] == 300);
}

TEST_CASE("vm_rs_copy_to_array - partial copy")
{
  Vm vm{};
  reset_vm(&vm);

  // Set up 5 values on return stack
  for (int i = 0; i < 5; i++)
  {
    vm.RS[i] = (i + 1) * 100;
  }
  vm.rp = vm.RS + 5;

  v4_i32 rs[3];
  int count = vm_rs_copy_to_array(&vm, rs, 3);

  CHECK(count == 3);  // Only max_count elements copied
  CHECK(rs[0] == 100);
  CHECK(rs[1] == 200);
  CHECK(rs[2] == 300);
}

TEST_CASE("vm_rs_copy_to_array - empty stack")
{
  Vm vm{};
  reset_vm(&vm);

  v4_i32 rs[256];
  int count = vm_rs_copy_to_array(&vm, rs, 256);

  CHECK(count == 0);
}

TEST_CASE("vm_rs_copy_to_array - NULL handling")
{
  Vm vm{};
  reset_vm(&vm);

  v4_i32 rs[256];

  CHECK(vm_rs_copy_to_array(nullptr, rs, 256) == 0);
  CHECK(vm_rs_copy_to_array(&vm, nullptr, 256) == 0);
  CHECK(vm_rs_copy_to_array(&vm, rs, 0) == 0);
  CHECK(vm_rs_copy_to_array(&vm, rs, -1) == 0);
}
