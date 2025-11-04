#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstdint>

#include "doctest.h"
#include "v4/errors.hpp"
#include "v4/internal/vm.h"
#include "v4/opcodes.hpp"
#include "v4/vm_api.h"

TEST_CASE("Panic handler: Empty stack")
{
  Vm vm{};
  vm_reset(&vm);

  // Trigger panic with empty stack
  v4_err err = vm_panic(&vm, static_cast<v4_err>(Err::StackUnderflow));

  CHECK(err == static_cast<v4_err>(Err::StackUnderflow));
  // Expected output:
  // Data Stack: [0]
}

TEST_CASE("Panic handler: With stack data")
{
  Vm vm{};
  vm_reset(&vm);

  // Push data to stack
  vm_ds_push(&vm, 42);
  vm_ds_push(&vm, 100);

  // Trigger panic
  v4_err err = vm_panic(&vm, static_cast<v4_err>(Err::InvalidArg));

  CHECK(err == static_cast<v4_err>(Err::InvalidArg));
  CHECK(vm_ds_depth_public(&vm) == 2);
  // Expected output:
  // TOS=100, NOS=42
}

TEST_CASE("Panic handler: Multiple stack items")
{
  Vm vm{};
  vm_reset(&vm);

  // Push multiple items
  for (int i = 1; i <= 5; i++)
  {
    vm_ds_push(&vm, i * 10);
  }

  // Trigger panic
  v4_err err = vm_panic(&vm, static_cast<v4_err>(Err::OobMemory));

  CHECK(err == static_cast<v4_err>(Err::OobMemory));
  CHECK(vm_ds_depth_public(&vm) == 5);
  // Expected output:
  // Data Stack: [5] TOS=50, NOS=40
}

TEST_CASE("Panic handler: Stack overflow detection")
{
  Vm vm{};
  vm_reset(&vm);

  // Intentionally overflow the stack
  v4_err err = 0;
  for (int i = 0; i < 1000; i++)
  {
    err = vm_ds_push(&vm, i);
    if (err < 0)
    {
      break;
    }
  }

  CHECK(err == static_cast<v4_err>(Err::StackOverflow));
  // vm_ds_push should have called vm_panic internally
}

TEST_CASE("Panic handler: Stack underflow detection")
{
  Vm vm{};
  vm_reset(&vm);

  // Pop from empty stack
  v4_i32 value;
  v4_err err = vm_ds_pop(&vm, &value);

  CHECK(err == static_cast<v4_err>(Err::StackUnderflow));
  // vm_ds_pop should have called vm_panic internally
}
