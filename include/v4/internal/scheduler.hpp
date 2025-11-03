#pragma once
#include "v4/internal/vm.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /* ========================================================================= */
  /* Scheduler Internal API                                                   */
  /* ========================================================================= */

  /**
   * @brief Save current task context
   *
   * Saves the execution state of current task to its TCB.
   *
   * @param vm VM instance
   * @param task Task control block
   */
  void v4_task_save_context(Vm *vm, v4_task_t *task);

  /**
   * @brief Restore task context
   *
   * Restores execution state from TCB to VM.
   *
   * @param vm VM instance
   * @param task Task control block
   */
  void v4_task_restore_context(Vm *vm, const v4_task_t *task);

  /**
   * @brief Select next task to run
   *
   * Priority-based scheduler with round-robin for equal priorities.
   * Wakes up sleeping tasks if their sleep time expired.
   *
   * @param vm VM instance
   * @return Next task ID to run
   */
  uint8_t v4_task_select_next(Vm *vm);

  /**
   * @brief Perform task scheduling
   *
   * Switches from current task to next task.
   * Called by timer interrupt or YIELD instruction.
   *
   * @param vm VM instance
   * @return 0 on success
   */
  v4_err vm_schedule(Vm *vm);

  /**
   * @brief Schedule from interrupt context
   *
   * ISR-safe version of vm_schedule().
   * Called from platform timer interrupt handler.
   *
   * @param vm VM instance
   * @return 0 on success
   */
  v4_err vm_schedule_from_isr(Vm *vm);

#ifdef __cplusplus
} /* extern "C" */
#endif
