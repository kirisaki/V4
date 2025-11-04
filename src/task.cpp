#include "v4/task.h"

#include "v4/errors.hpp"
#include "v4/internal/task_backend.h"
#include "v4/internal/vm.h"
#include "v4/task_platform.h"

/* ========================================================================= */
/* Task Management API (Backend Wrapper)                                     */
/* ========================================================================= */
/**
 * This file provides the public task management API.
 * All implementation details are delegated to the backend layer.
 */

extern "C" v4_err vm_task_init(Vm *vm, uint32_t time_slice_ms)
{
  return v4_backend_task_init(vm, time_slice_ms);
}

extern "C" v4_err vm_task_cleanup(Vm *vm)
{
  return v4_backend_task_cleanup(vm);
}

extern "C" int vm_task_spawn(Vm *vm, uint16_t word_idx, uint8_t priority,
                             uint16_t ds_size, uint16_t rs_size)
{
  return v4_backend_task_spawn(vm, word_idx, priority, ds_size, rs_size);
}

extern "C" v4_err vm_task_exit(Vm *vm)
{
  return v4_backend_task_exit(vm);
}

extern "C" v4_err vm_task_sleep(Vm *vm, uint32_t ms_delay)
{
  return v4_backend_task_sleep(vm, ms_delay);
}

extern "C" v4_err vm_task_yield(Vm *vm)
{
  return v4_backend_task_yield(vm);
}

extern "C" v4_err vm_task_critical_enter(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  v4_task_platform_critical_enter();
  vm->scheduler.critical_nesting++;

  return V4_ERR(OK);
}

extern "C" v4_err vm_task_critical_exit(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  if (vm->scheduler.critical_nesting > 0)
  {
    vm->scheduler.critical_nesting--;
    if (vm->scheduler.critical_nesting == 0)
    {
      v4_task_platform_critical_exit();
    }
  }

  return V4_ERR(OK);
}

extern "C" int vm_task_self(Vm *vm)
{
  return v4_backend_task_current(vm);
}

extern "C" v4_err vm_task_get_info(Vm *vm, uint8_t task_id, v4_task_state_t *state,
                                   uint8_t *priority)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  uint8_t task_state = 0;
  v4_err err = v4_backend_task_get_state(vm, task_id, &task_state);
  if (err != V4_ERR(OK))
    return err;

  if (state)
    *state = static_cast<v4_task_state_t>(task_state);

  // Priority retrieval is not yet abstracted
  if (priority && task_id < V4_MAX_TASKS)
    *priority = vm->scheduler.tasks[task_id].priority;

  return V4_ERR(OK);
}
