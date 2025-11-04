#pragma once
#include <stdint.h>

#include "v4/vm_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /* ========================================================================= */
  /* Task Backend Interface                                                    */
  /* ========================================================================= */
  /**
   * This interface abstracts the task management implementation.
   * Allows switching between custom scheduler and FreeRTOS at compile time.
   */

  /**
   * @brief Backend-specific initialization
   *
   * @param vm VM instance
   * @param time_slice_ms Time slice in milliseconds (may be ignored by some backends)
   * @return 0 on success, negative on error
   */
  v4_err v4_backend_task_init(struct Vm *vm, uint32_t time_slice_ms);

  /**
   * @brief Backend-specific cleanup
   *
   * @param vm VM instance
   * @return 0 on success
   */
  v4_err v4_backend_task_cleanup(struct Vm *vm);

  /**
   * @brief Spawn a new task (backend implementation)
   *
   * @param vm VM instance
   * @param word_idx Word index to execute
   * @param priority Priority (0=lowest, 255=highest)
   * @param ds_size Data stack size in elements
   * @param rs_size Return stack size in elements
   * @return Task ID on success, negative on error
   */
  int v4_backend_task_spawn(struct Vm *vm, uint16_t word_idx, uint8_t priority,
                            uint16_t ds_size, uint16_t rs_size);

  /**
   * @brief Exit current task (backend implementation)
   *
   * @param vm VM instance
   * @return 0 on success
   */
  v4_err v4_backend_task_exit(struct Vm *vm);

  /**
   * @brief Sleep current task (backend implementation)
   *
   * @param vm VM instance
   * @param ms_delay Sleep duration in milliseconds
   * @return 0 on success
   */
  v4_err v4_backend_task_sleep(struct Vm *vm, uint32_t ms_delay);

  /**
   * @brief Yield CPU to next task (backend implementation)
   *
   * @param vm VM instance
   * @return 0 on success
   */
  v4_err v4_backend_task_yield(struct Vm *vm);

  /**
   * @brief Get current task ID (backend implementation)
   *
   * @param vm VM instance
   * @return Current task ID (0-7), or 0 if single-threaded
   */
  uint8_t v4_backend_task_current(struct Vm *vm);

  /**
   * @brief Get task state (backend implementation)
   *
   * @param vm VM instance
   * @param task_id Task ID
   * @param state Output: task state
   * @return 0 on success, negative on error
   */
  v4_err v4_backend_task_get_state(struct Vm *vm, uint8_t task_id, uint8_t *state);

  /**
   * @brief Send message (backend implementation)
   *
   * @param vm VM instance
   * @param dst_task Destination task ID (0xFF = broadcast)
   * @param msg_type Message type
   * @param data Data payload
   * @return 0 on success, negative on error
   */
  v4_err v4_backend_task_send(struct Vm *vm, uint8_t dst_task, uint8_t msg_type, int32_t data);

  /**
   * @brief Receive message (non-blocking, backend implementation)
   *
   * @param vm VM instance
   * @param msg_type Message type (0 = any type)
   * @param data Output: data payload
   * @param src_task Output: source task ID
   * @return 1 if message received, 0 if queue empty, negative on error
   */
  int v4_backend_task_receive(struct Vm *vm, uint8_t msg_type, int32_t *data, uint8_t *src_task);

  /**
   * @brief Receive message (blocking, backend implementation)
   *
   * @param vm VM instance
   * @param msg_type Message type (0 = any type)
   * @param data Output: data payload
   * @param src_task Output: source task ID
   * @param timeout_ms Timeout in milliseconds (0 = wait forever)
   * @return 1 if message received, 0 on timeout, negative on error
   */
  int v4_backend_task_receive_blocking(struct Vm *vm, uint8_t msg_type, int32_t *data,
                                       uint8_t *src_task, uint32_t timeout_ms);

  /**
   * @brief Perform scheduling (backend implementation)
   *
   * Called by platform timer interrupt or yield.
   *
   * @param vm VM instance
   * @return 0 on success
   */
  v4_err v4_backend_schedule(struct Vm *vm);

  /**
   * @brief Schedule from ISR (backend implementation)
   *
   * ISR-safe version of scheduling.
   *
   * @param vm VM instance
   * @return 0 on success
   */
  v4_err v4_backend_schedule_from_isr(struct Vm *vm);

#ifdef __cplusplus
} /* extern "C" */
#endif
