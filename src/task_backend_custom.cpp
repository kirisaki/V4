#include <cstdlib>
#include <cstring>

#include "v4/errors.hpp"
#include "v4/internal/scheduler.hpp"
#include "v4/internal/task_backend.h"
#include "v4/internal/vm.h"
#include "v4/task.h"
#include "v4/task_platform.h"

/* ========================================================================= */
/* Custom Task Backend Implementation                                        */
/* ========================================================================= */
/**
 * This file implements the task backend interface using V4-engine's
 * custom scheduler (priority-based + round-robin).
 */

extern "C" v4_err v4_backend_task_init(Vm *vm, uint32_t time_slice_ms)
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

extern "C" v4_err v4_backend_task_cleanup(Vm *vm)
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

extern "C" int v4_backend_task_spawn(Vm *vm, uint16_t word_idx, uint8_t priority,
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

extern "C" v4_err v4_backend_task_exit(Vm *vm)
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

extern "C" v4_err v4_backend_task_sleep(Vm *vm, uint32_t ms_delay)
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

extern "C" v4_err v4_backend_task_yield(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Simply trigger scheduling
  vm_schedule(vm);

  return V4_ERR(OK);
}

extern "C" uint8_t v4_backend_task_current(Vm *vm)
{
  if (!vm)
    return 0;

  return vm->scheduler.current_task;
}

extern "C" v4_err v4_backend_task_get_state(Vm *vm, uint8_t task_id, uint8_t *state)
{
  if (!vm || task_id >= V4_MAX_TASKS)
    return V4_ERR(InvalidArg);

  v4_task_t *task = &vm->scheduler.tasks[task_id];

  if (state)
    *state = task->state;

  return V4_ERR(OK);
}

/* ========================================================================= */
/* Message Passing Backend Implementation                                    */
/* ========================================================================= */

extern "C" v4_err v4_backend_task_send(Vm *vm, uint8_t dst_task, uint8_t msg_type,
                                       int32_t data)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Validate target task (allow 0xFF for broadcast)
  if (dst_task >= V4_MAX_TASKS && dst_task != 0xFF)
    return -2;

  v4_msg_queue_t *queue = &vm->msg_queue;

  // Check if queue is full
  if (queue->count >= V4_MSG_QUEUE_SIZE)
    return -1;

  // Create message
  v4_message_t *msg = &queue->queue[queue->write_idx];
  msg->src_task = vm->scheduler.current_task;
  msg->dst_task = dst_task;
  msg->msg_type = msg_type;
  msg->data = data;
  msg->flags = 0;

  // Update ring buffer indices
  queue->write_idx = (queue->write_idx + 1) % V4_MSG_QUEUE_SIZE;
  queue->count++;

  return V4_ERR(OK);
}

extern "C" int v4_backend_task_receive(Vm *vm, uint8_t msg_type, int32_t *data,
                                       uint8_t *src_task)
{
  if (!vm)
    return -1;

  v4_msg_queue_t *queue = &vm->msg_queue;
  uint8_t current_task = vm->scheduler.current_task;

  // Search queue for matching message
  for (uint8_t i = 0; i < queue->count; i++)
  {
    uint8_t idx = (queue->read_idx + i) % V4_MSG_QUEUE_SIZE;
    v4_message_t *msg = &queue->queue[idx];

    // Check if message matches:
    // - Addressed to this task or broadcast (0xFF)
    // - Message type matches (or msg_type == 0 for any)
    if ((msg->dst_task == current_task || msg->dst_task == 0xFF) &&
        (msg_type == 0 || msg->msg_type == msg_type))
    {
      // Copy message data
      if (data)
        *data = msg->data;
      if (src_task)
        *src_task = msg->src_task;

      // Remove message from queue by shifting
      // This maintains message order
      for (uint8_t j = i; j < queue->count - 1; j++)
      {
        uint8_t src_idx = (queue->read_idx + j + 1) % V4_MSG_QUEUE_SIZE;
        uint8_t dst_idx = (queue->read_idx + j) % V4_MSG_QUEUE_SIZE;
        queue->queue[dst_idx] = queue->queue[src_idx];
      }
      queue->count--;

      return 1;  // Message received
    }
  }

  return 0;  // No message
}

extern "C" int v4_backend_task_receive_blocking(Vm *vm, uint8_t msg_type, int32_t *data,
                                                uint8_t *src_task, uint32_t timeout_ms)
{
  if (!vm)
    return -1;

  uint32_t start_tick = v4_task_platform_get_tick_ms();

  while (true)
  {
    // Try non-blocking receive
    int result = v4_backend_task_receive(vm, msg_type, data, src_task);
    if (result == 1)
      return 1;  // Message received

    // Check timeout
    if (timeout_ms > 0)
    {
      uint32_t current_tick = v4_task_platform_get_tick_ms();
      uint32_t elapsed = current_tick - start_tick;
      if (elapsed >= timeout_ms)
        return 0;  // Timeout
    }

    // Sleep briefly and yield to other tasks
    v4_backend_task_sleep(vm, 10);
  }
}

/* ========================================================================= */
/* Scheduler Backend Implementation                                          */
/* ========================================================================= */

extern "C" v4_err v4_backend_schedule(Vm *vm)
{
  // Delegate to existing scheduler implementation
  return vm_schedule(vm);
}

extern "C" v4_err v4_backend_schedule_from_isr(Vm *vm)
{
  // Delegate to existing ISR scheduler implementation
  return vm_schedule_from_isr(vm);
}
