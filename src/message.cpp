#include "v4/errors.hpp"
#include "v4/internal/vm.h"
#include "v4/task.h"
#include "v4/task_platform.h"

/* ========================================================================= */
/* Message Passing Implementation                                            */
/* ========================================================================= */

extern "C" v4_err vm_task_send(Vm *vm, uint8_t target_task, uint8_t msg_type,
                               int32_t data)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Validate target task (allow 0xFF for broadcast)
  if (target_task >= V4_MAX_TASKS && target_task != 0xFF)
    return -2;

  v4_msg_queue_t *queue = &vm->msg_queue;

  // Check if queue is full
  if (queue->count >= V4_MSG_QUEUE_SIZE)
    return -1;

  // Create message
  v4_message_t *msg = &queue->queue[queue->write_idx];
  msg->src_task = vm->scheduler.current_task;
  msg->dst_task = target_task;
  msg->msg_type = msg_type;
  msg->data = data;
  msg->flags = 0;

  // Update ring buffer indices
  queue->write_idx = (queue->write_idx + 1) % V4_MSG_QUEUE_SIZE;
  queue->count++;

  return V4_ERR(OK);
}

extern "C" int vm_task_receive(Vm *vm, uint8_t msg_type, int32_t *data, uint8_t *src_task)
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

extern "C" int vm_task_receive_blocking(Vm *vm, uint8_t msg_type, int32_t *data,
                                        uint8_t *src_task, uint32_t timeout_ms)
{
  if (!vm)
    return -1;

  uint32_t start_tick = v4_task_platform_get_tick_ms();

  while (true)
  {
    // Try non-blocking receive
    int result = vm_task_receive(vm, msg_type, data, src_task);
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
    vm_task_sleep(vm, 10);
  }
}
