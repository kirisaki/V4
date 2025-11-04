#include <cstring>

#include "v4/errors.hpp"
#include "v4/internal/task_backend.h"
#include "v4/internal/vm.h"

// FreeRTOS headers (ESP-IDF uses different include paths)
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#endif

/* ========================================================================= */
/* FreeRTOS Task Backend Implementation                                      */
/* ========================================================================= */
/**
 * This file implements the task backend interface using FreeRTOS.
 * Provides integration with FreeRTOS RTOS for embedded platforms.
 */

// V4 task ID to FreeRTOS task handle mapping
static TaskHandle_t task_handles[V4_MAX_TASKS] = {NULL};

// Message queue for inter-task communication
static QueueHandle_t msg_queue = NULL;

// Priority mapping: V4 priority (0-255) to FreeRTOS priority
#define V4_TO_FREERTOS_PRIORITY(v4_prio) ((v4_prio * (configMAX_PRIORITIES - 1)) / 255)

/* ========================================================================= */
/* Task Backend Implementation                                               */
/* ========================================================================= */

extern "C" v4_err v4_backend_task_init(Vm *vm, uint32_t time_slice_ms)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Initialize VM scheduler structure
  memset(&vm->scheduler, 0, sizeof(v4_scheduler_t));
  vm->scheduler.time_slice_ms = (time_slice_ms > 0) ? time_slice_ms : 10;
  vm->scheduler.current_task = 0;
  vm->scheduler.task_count = 0;

  // Initialize task handles
  for (int i = 0; i < V4_MAX_TASKS; i++)
  {
    task_handles[i] = NULL;
    vm->scheduler.tasks[i].state = V4_TASK_STATE_DEAD;
  }

  // Create message queue (16 messages, each holding v4_message_t)
  msg_queue = xQueueCreate(V4_MSG_QUEUE_SIZE, sizeof(v4_message_t));
  if (msg_queue == NULL)
    return V4_ERR(NoMemory);

  return V4_ERR(OK);
}

extern "C" v4_err v4_backend_task_cleanup(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Delete all FreeRTOS tasks
  for (int i = 0; i < V4_MAX_TASKS; i++)
  {
    if (task_handles[i] != NULL)
    {
      vTaskDelete(task_handles[i]);
      task_handles[i] = NULL;
    }
    vm->scheduler.tasks[i].state = V4_TASK_STATE_DEAD;
  }

  // Delete message queue
  if (msg_queue != NULL)
  {
    vQueueDelete(msg_queue);
    msg_queue = NULL;
  }

  vm->scheduler.task_count = 0;

  return V4_ERR(OK);
}

// FreeRTOS task wrapper function
static void v4_freertos_task_wrapper(void *pvParameters)
{
  // Task parameter contains VM and word index
  struct
  {
    Vm *vm;
    uint16_t word_idx;
    uint8_t task_id;
  } *params = (decltype(params))pvParameters;

  Vm *vm = params->vm;
  uint16_t word_idx = params->word_idx;
  uint8_t task_id = params->task_id;

  // Mark task as running
  vm->scheduler.current_task = task_id;
  vm->scheduler.tasks[task_id].state = V4_TASK_STATE_RUNNING;

  // Execute the word (this should be an infinite loop in V4 Forth)
  // Note: In a real implementation, vm_exec() would be called here
  // vm_exec(vm, word_idx);

  // If task exits, mark as DEAD
  vm->scheduler.tasks[task_id].state = V4_TASK_STATE_DEAD;
  vm->scheduler.task_count--;

  // Delete self
  vTaskDelete(NULL);
}

extern "C" int v4_backend_task_spawn(Vm *vm, uint16_t word_idx, uint8_t priority,
                                     uint16_t ds_size, uint16_t rs_size)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Validate word index
  if (word_idx >= vm->word_count)
    return V4_ERR(InvalidWordIdx);

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
    return V4_ERR(TaskLimit);

  v4_task_t *task = &vm->scheduler.tasks[task_id];

  // Apply defaults
  if (ds_size == 0)
    ds_size = 256;
  if (rs_size == 0)
    rs_size = 64;

  // Initialize task state
  task->word_idx = word_idx;
  task->priority = priority;
  task->ds_size = ds_size;
  task->rs_size = rs_size;
  task->state = V4_TASK_STATE_READY;

  // Prepare task parameters
  struct
  {
    Vm *vm;
    uint16_t word_idx;
    uint8_t task_id;
  } *params = (decltype(params))pvPortMalloc(sizeof(*params));

  if (!params)
    return V4_ERR(NoMemory);

  params->vm = vm;
  params->word_idx = word_idx;
  params->task_id = task_id;

  // Create FreeRTOS task
  UBaseType_t freertos_priority = V4_TO_FREERTOS_PRIORITY(priority);
  BaseType_t result =
      xTaskCreate(v4_freertos_task_wrapper, "V4Task",
                  (ds_size + rs_size) * sizeof(int32_t) / sizeof(StackType_t), params,
                  freertos_priority, &task_handles[task_id]);

  if (result != pdPASS)
  {
    vPortFree(params);
    task->state = V4_TASK_STATE_DEAD;
    return V4_ERR(NoMemory);
  }

  vm->scheduler.task_count++;

  return task_id;
}

extern "C" v4_err v4_backend_task_exit(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  uint8_t task_id = vm->scheduler.current_task;
  v4_task_t *task = &vm->scheduler.tasks[task_id];

  // Mark task as DEAD
  task->state = V4_TASK_STATE_DEAD;
  vm->scheduler.task_count--;

  // Delete FreeRTOS task (self)
  if (task_handles[task_id] != NULL)
  {
    task_handles[task_id] = NULL;
    vTaskDelete(NULL);  // Does not return
  }

  return V4_ERR(OK);
}

extern "C" v4_err v4_backend_task_sleep(Vm *vm, uint32_t ms_delay)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  uint8_t task_id = vm->scheduler.current_task;
  v4_task_t *task = &vm->scheduler.tasks[task_id];

  // Mark task as blocked
  task->state = V4_TASK_STATE_BLOCKED;

  // Convert milliseconds to ticks
  TickType_t ticks = pdMS_TO_TICKS(ms_delay);
  vTaskDelay(ticks);

  // Task is READY again after waking up
  task->state = V4_TASK_STATE_READY;

  return V4_ERR(OK);
}

extern "C" v4_err v4_backend_task_yield(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // Yield to FreeRTOS scheduler
  taskYIELD();

  return V4_ERR(OK);
}

extern "C" uint8_t v4_backend_task_current(Vm *vm)
{
  if (!vm)
    return 0;

  // Find current task by matching FreeRTOS task handle
  TaskHandle_t current = xTaskGetCurrentTaskHandle();

  for (int i = 0; i < V4_MAX_TASKS; i++)
  {
    if (task_handles[i] == current)
      return i;
  }

  return 0;  // Default to task 0
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
    return V4_ERR(InvalidArg);

  if (msg_queue == NULL)
    return V4_ERR(InvalidArg);

  // Create message
  v4_message_t msg;
  msg.src_task = v4_backend_task_current(vm);
  msg.dst_task = dst_task;
  msg.msg_type = msg_type;
  msg.data = data;
  msg.flags = 0;

  // Send to FreeRTOS queue (non-blocking)
  BaseType_t result = xQueueSend(msg_queue, &msg, 0);
  if (result != pdPASS)
    return V4_ERR(MsgQueueFull);

  return V4_ERR(OK);
}

extern "C" int v4_backend_task_receive(Vm *vm, uint8_t msg_type, int32_t *data,
                                       uint8_t *src_task)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  if (msg_queue == NULL)
    return 0;

  uint8_t current_task = v4_backend_task_current(vm);

  // Try to receive from FreeRTOS queue (non-blocking)
  v4_message_t msg;
  BaseType_t result = xQueueReceive(msg_queue, &msg, 0);

  if (result == pdPASS)
  {
    // Check if message matches criteria
    if ((msg.dst_task == current_task || msg.dst_task == 0xFF) &&
        (msg_type == 0 || msg.msg_type == msg_type))
    {
      if (data)
        *data = msg.data;
      if (src_task)
        *src_task = msg.src_task;

      return 1;  // Message received
    }
    else
    {
      // Message doesn't match, put it back (requires queue management)
      // TODO: Implement proper message filtering with peek
      return 0;
    }
  }

  return 0;  // No message
}

extern "C" int v4_backend_task_receive_blocking(Vm *vm, uint8_t msg_type, int32_t *data,
                                                uint8_t *src_task, uint32_t timeout_ms)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  if (msg_queue == NULL)
    return 0;

  uint8_t current_task = v4_backend_task_current(vm);
  TickType_t ticks = (timeout_ms > 0) ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY;

  // Receive from FreeRTOS queue (blocking with timeout)
  v4_message_t msg;
  BaseType_t result = xQueueReceive(msg_queue, &msg, ticks);

  if (result == pdPASS)
  {
    // Check if message matches criteria
    if ((msg.dst_task == current_task || msg.dst_task == 0xFF) &&
        (msg_type == 0 || msg.msg_type == msg_type))
    {
      if (data)
        *data = msg.data;
      if (src_task)
        *src_task = msg.src_task;

      return 1;  // Message received
    }
    else
    {
      // Message doesn't match, put it back
      // TODO: Implement proper message filtering
      return 0;
    }
  }

  return 0;  // Timeout or no message
}

/* ========================================================================= */
/* Scheduler Backend Implementation                                          */
/* ========================================================================= */

extern "C" v4_err v4_backend_schedule(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // FreeRTOS handles scheduling automatically
  // Just yield to trigger scheduler
  taskYIELD();

  return V4_ERR(OK);
}

extern "C" v4_err v4_backend_schedule_from_isr(Vm *vm)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  // FreeRTOS handles ISR scheduling
  // Use taskYIELD_FROM_ISR() or portYIELD_FROM_ISR()
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

  return V4_ERR(OK);
}
