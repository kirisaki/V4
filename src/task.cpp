#include "v4/task.h"

#include <cstdlib>
#include <cstring>

#include "v4/errors.hpp"
#include "v4/internal/scheduler.hpp"
#include "v4/internal/vm.h"
#include "v4/task_platform.h"

/* ========================================================================= */
/* Task Management Implementation                                            */
/* ========================================================================= */

extern "C" v4_err vm_task_init(Vm *vm, uint32_t time_slice_ms)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Initialize scheduler
  memset(&vm->scheduler, 0, sizeof(v4_scheduler_t));
  vm->scheduler.time_slice_ms = (time_slice_ms > 0) ? time_slice_ms : 10;
  vm->scheduler.current_task = 0;
  vm->scheduler.task_count = 0;

  // Initialize message queue
  memset(&vm->msg_queue, 0, sizeof(v4_msg_queue_t));

  // Mark all tasks as DEAD
  for (int i = 0; i < V4_MAX_TASKS; i++)
  {
    vm->scheduler.tasks[i].state = V4_TASK_STATE_DEAD;
    vm->scheduler.tasks[i].ds_base = nullptr;
    vm->scheduler.tasks[i].rs_base = nullptr;
  }

  return V4_ERR(OK);
}

extern "C" v4_err vm_task_cleanup(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Free all task stacks
  for (int i = 0; i < V4_MAX_TASKS; i++)
  {
    v4_task_t *task = &vm->scheduler.tasks[i];
    if (task->ds_base)
    {
      free(task->ds_base);
      task->ds_base = nullptr;
    }
    if (task->rs_base)
    {
      free(task->rs_base);
      task->rs_base = nullptr;
    }
    task->state = V4_TASK_STATE_DEAD;
  }

  // Reset scheduler
  vm->scheduler.task_count = 0;

  // Reset message queue
  memset(&vm->msg_queue, 0, sizeof(v4_msg_queue_t));

  return V4_ERR(OK);
}

extern "C" int vm_task_spawn(Vm *vm, uint16_t word_idx, uint8_t priority,
                             uint16_t ds_size, uint16_t rs_size)
{
  if (!vm)
    return -3;

  // Validate word index
  if (word_idx >= vm->word_count)
    return -3;

  // Find free task slot
  int task_id = -1;
  for (int i = 0; i < V4_MAX_TASKS; i++)
  {
    if (vm->scheduler.tasks[i].state == V4_TASK_STATE_DEAD)
    {
      task_id = i;
      break;
    }
  }

  if (task_id < 0)
    return -1;  // Task table full

  v4_task_t *task = &vm->scheduler.tasks[task_id];

  // Apply defaults
  if (ds_size == 0)
    ds_size = 256;
  if (rs_size == 0)
    rs_size = 64;

  // Allocate independent stacks for task
  task->ds_base = static_cast<int32_t *>(malloc(ds_size * sizeof(int32_t)));
  task->rs_base = static_cast<int32_t *>(malloc(rs_size * sizeof(int32_t)));

  if (!task->ds_base || !task->rs_base)
  {
    // Cleanup on allocation failure
    if (task->ds_base)
      free(task->ds_base);
    if (task->rs_base)
      free(task->rs_base);
    task->ds_base = nullptr;
    task->rs_base = nullptr;
    return -2;  // Out of memory
  }

  // Initialize task state
  task->word_idx = word_idx;
  task->pc = 0;
  task->ds_depth = 0;
  task->rs_depth = 0;
  task->state = V4_TASK_STATE_READY;
  task->priority = priority;
  task->sleep_until_tick = 0;
  task->ds_size = ds_size;
  task->rs_size = rs_size;
  task->exec_count = 0;

  vm->scheduler.task_count++;

  return task_id;
}

extern "C" v4_err vm_task_exit(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  uint8_t task_id = vm->scheduler.current_task;
  v4_task_t *task = &vm->scheduler.tasks[task_id];

  // Free task stacks
  if (task->ds_base)
  {
    free(task->ds_base);
    task->ds_base = nullptr;
  }
  if (task->rs_base)
  {
    free(task->rs_base);
    task->rs_base = nullptr;
  }

  // Mark task as DEAD
  task->state = V4_TASK_STATE_DEAD;
  vm->scheduler.task_count--;

  // Schedule next task (does not return)
  vm_schedule(vm);

  return V4_ERR(OK);  // Never reached
}

extern "C" v4_err vm_task_sleep(Vm *vm, uint32_t ms_delay)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  uint8_t task_id = vm->scheduler.current_task;
  v4_task_t *task = &vm->scheduler.tasks[task_id];

  // Set sleep deadline
  uint32_t current_tick = v4_task_platform_get_tick_ms();
  task->sleep_until_tick = current_tick + ms_delay;
  task->state = V4_TASK_STATE_BLOCKED;

  // Yield to next task
  vm_schedule(vm);

  return V4_ERR(OK);
}

extern "C" v4_err vm_task_yield(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Simply trigger scheduling
  vm_schedule(vm);

  return V4_ERR(OK);
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
  if (!vm)
    return -1;

  return vm->scheduler.current_task;
}

extern "C" v4_err vm_task_get_info(Vm *vm, uint8_t task_id, v4_task_state_t *state,
                                   uint8_t *priority)
{
  if (!vm || task_id >= V4_MAX_TASKS)
    return V4_ERR(InvalidArg);

  v4_task_t *task = &vm->scheduler.tasks[task_id];

  if (state)
    *state = static_cast<v4_task_state_t>(task->state);
  if (priority)
    *priority = task->priority;

  return V4_ERR(OK);
}
