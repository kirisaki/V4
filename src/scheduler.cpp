#include "v4/internal/scheduler.hpp"
#include "v4/errors.hpp"
#include "v4/internal/vm.h"
#include "v4/task_platform.h"
#include <cstring>

/* ========================================================================= */
/* Scheduler Core Implementation                                             */
/* ========================================================================= */

extern "C" void v4_task_save_context(Vm *vm, v4_task_t *task)
{
  if (!vm || !task)
    return;

  // Save stack depths
  task->ds_depth = static_cast<uint8_t>(vm->sp - vm->DS);
  task->rs_depth = static_cast<uint8_t>(vm->rp - vm->RS);

  // Copy stack contents to task's independent stacks
  if (task->ds_base && task->ds_depth > 0)
  {
    memcpy(task->ds_base, vm->DS, task->ds_depth * sizeof(int32_t));
  }
  if (task->rs_base && task->rs_depth > 0)
  {
    memcpy(task->rs_base, vm->RS, task->rs_depth * sizeof(int32_t));
  }

  // Note: PC is not saved - tasks run continuously from their word entry point
  // This is sufficient for preemptive multitasking where tasks are typically
  // infinite loops that call YIELD or SLEEP periodically
}

extern "C" void v4_task_restore_context(Vm *vm, const v4_task_t *task)
{
  if (!vm || !task)
    return;

  // Restore stack contents from task's independent stacks
  if (task->ds_base && task->ds_depth > 0)
  {
    memcpy(vm->DS, task->ds_base, task->ds_depth * sizeof(int32_t));
  }
  if (task->rs_base && task->rs_depth > 0)
  {
    memcpy(vm->RS, task->rs_base, task->rs_depth * sizeof(int32_t));
  }

  // Restore stack pointers
  vm->sp = vm->DS + task->ds_depth;
  vm->rp = vm->RS + task->rs_depth;

  // Note: PC restoration is handled by vm_exec() when it resumes
}

extern "C" uint8_t v4_task_select_next(Vm *vm)
{
  if (!vm)
    return 0;

  v4_scheduler_t *sched = &vm->scheduler;
  uint32_t current_tick = v4_task_platform_get_tick_ms();

  uint8_t highest_priority = 0;
  uint8_t selected_task = 0xFF;

  // Priority-based scheduling with round-robin for equal priorities
  for (uint8_t i = 0; i < V4_MAX_TASKS; i++)
  {
    v4_task_t *task = &sched->tasks[i];

    // Skip DEAD tasks
    if (task->state == V4_TASK_STATE_DEAD)
      continue;

    // Wake up sleeping tasks if deadline passed
    if (task->state == V4_TASK_STATE_BLOCKED)
    {
      if (current_tick >= task->sleep_until_tick)
      {
        task->state = V4_TASK_STATE_READY;
      }
      else
      {
        continue;  // Still sleeping
      }
    }

    // Select highest priority READY task
    if (task->state == V4_TASK_STATE_READY || task->state == V4_TASK_STATE_RUNNING)
    {
      if (task->priority > highest_priority)
      {
        highest_priority = task->priority;
        selected_task = i;
      }
      else if (task->priority == highest_priority && selected_task != 0xFF)
      {
        // Round-robin: prefer tasks after current task
        if (i > sched->current_task && selected_task <= sched->current_task)
        {
          selected_task = i;
        }
      }
    }
  }

  // If no task found, keep current task
  if (selected_task == 0xFF)
  {
    selected_task = sched->current_task;
  }

  return selected_task;
}

extern "C" v4_err vm_schedule(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  v4_scheduler_t *sched = &vm->scheduler;

  // Save current task context
  v4_task_t *current = &sched->tasks[sched->current_task];
  if (current->state == V4_TASK_STATE_RUNNING)
  {
    v4_task_save_context(vm, current);
    current->state = V4_TASK_STATE_READY;
  }

  // Select next task
  uint8_t next_task_id = v4_task_select_next(vm);

  // No switch needed
  if (next_task_id == sched->current_task &&
      current->state != V4_TASK_STATE_DEAD &&
      current->state != V4_TASK_STATE_BLOCKED)
  {
    current->state = V4_TASK_STATE_RUNNING;
    return V4_ERR(OK);
  }

  // Context switch
  v4_task_t *next = &sched->tasks[next_task_id];
  v4_task_restore_context(vm, next);
  next->state = V4_TASK_STATE_RUNNING;
  next->exec_count++;

  sched->current_task = next_task_id;
  sched->context_switches++;

  return V4_ERR(OK);
}

extern "C" v4_err vm_schedule_from_isr(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Don't preempt if in critical section
  if (vm->scheduler.critical_nesting > 0)
    return V4_ERR(OK);

  // Increment preemption counter
  vm->scheduler.preemptions++;

  // Perform normal scheduling
  return vm_schedule(vm);
}
