#include "v4/panic.h"

#include <inttypes.h>
#include <stdio.h>

#include "v4/errors.hpp"
#include "v4/internal/vm.h"  // For Vm struct definition
#include "v4/panic.hpp"
#include "v4/vm_api.h"

extern "C"
{
  void vm_set_panic_handler(struct Vm *vm, V4PanicHandler handler, void *user_data)
  {
    if (!vm)
      return;

    vm->panic_handler = handler;
    vm->panic_user_data = user_data;
  }

  v4_err vm_panic(struct Vm *vm, v4_err error_code)
  {
    using namespace v4;

    // Collect panic information
    V4PanicInfo info;
    info.error_code = error_code;
    info.pc = 0;  // TODO: Get PC from VM (need to expose PC in public API)
    info.ds_depth = vm_ds_depth_public(vm);
    info.rs_depth = vm_rs_depth_public(vm);
    info.has_stack_data = (info.ds_depth > 0);

    // Get top 2 elements from Data Stack
    if (info.has_stack_data)
    {
      if (info.ds_depth >= 1)
      {
        info.tos = vm_ds_peek_public(vm, 0);
      }
      if (info.ds_depth >= 2)
      {
        info.nos = vm_ds_peek_public(vm, 1);
      }
    }

    // Get top 4 stack values for custom handlers
    for (int i = 0; i < 4 && i < info.ds_depth; ++i)
    {
      info.stack[i] = vm_ds_peek_public(vm, i);
    }

    // Output diagnostic information
    printf("\n");
    printf("========== V4 PANIC ==========\n");

    // Error information
    Err err = static_cast<Err>(error_code);
    printf("Error: %s (code=%d)\n", err_str(err), error_code);

    // Program Counter
    printf("PC: 0x%08" PRIX32 "\n", info.pc);

    // Data Stack
    printf("Data Stack: [%d]", info.ds_depth);
    if (info.has_stack_data)
    {
      printf(" TOS=%" PRId32, info.tos);
      if (info.ds_depth >= 2)
      {
        printf(", NOS=%" PRId32, info.nos);
      }
    }
    printf("\n");

    // Return Stack with call trace
    printf("Return Stack: [%d]\n", info.rs_depth);
    if (info.rs_depth > 0)
    {
      printf("Call trace:\n");

      // Get Return Stack contents (up to 16 entries)
      v4_i32 rs_data[16];
      int count = vm_rs_copy_to_array(vm, rs_data, 16);

      for (int i = 0; i < count; i++)
      {
        printf("  [%d] 0x%08" PRIX32 "\n", i, static_cast<uint32_t>(rs_data[i]));
      }

      if (info.rs_depth > 16)
      {
        printf("  ... (%d more entries)\n", info.rs_depth - 16);
      }
    }

    printf("==============================\n");
    printf("\n");

    // Call custom panic handler if registered
    if (vm->panic_handler)
    {
      vm->panic_handler(vm->panic_user_data, &info);
    }

    return error_code;
  }

}  // extern "C"
