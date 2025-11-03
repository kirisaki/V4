#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /* Forward declaration */
  struct Vm;

  /* ========================================================================= */
  /* Platform Abstraction API                                                 */
  /* ========================================================================= */
  /**
   * These functions must be implemented by the platform-specific code
   * (V4-ports). They provide timer and interrupt control functionality.
   */

  /**
   * @brief Platform-specific timer initialization
   *
   * Configures a timer to interrupt at the specified time slice interval.
   * The interrupt handler must call vm_schedule_from_isr(vm).
   *
   * Example (ESP32-C6):
   *   Use esp_timer_create() and esp_timer_start_periodic()
   *   In callback, call vm_schedule_from_isr()
   *
   * Example (CH32V203):
   *   Use SysTick_Config() for SysTick setup
   *   In SysTick_Handler(), call vm_schedule_from_isr()
   *
   * @param vm VM instance (for use in interrupt handler)
   * @param time_slice_ms Time slice in milliseconds
   * @return 0 on success, negative on error
   */
  int v4_task_platform_init(struct Vm *vm, uint32_t time_slice_ms);

  /**
   * @brief Platform-specific timer cleanup
   *
   * Stops the timer interrupt and releases resources.
   *
   * @param vm VM instance
   * @return 0 on success
   */
  int v4_task_platform_deinit(struct Vm *vm);

  /**
   * @brief Enter critical section (disable interrupts)
   *
   * Disables global interrupts.
   * Must support nesting.
   *
   * Example (ARM Cortex-M):
   *   __disable_irq();
   *
   * Example (RISC-V):
   *   clear_csr(mstatus, MSTATUS_MIE);
   */
  void v4_task_platform_critical_enter(void);

  /**
   * @brief Exit critical section (enable interrupts)
   *
   * Re-enables global interrupts.
   *
   * Example (ARM Cortex-M):
   *   __enable_irq();
   *
   * Example (RISC-V):
   *   set_csr(mstatus, MSTATUS_MIE);
   */
  void v4_task_platform_critical_exit(void);

  /**
   * @brief Get system tick count (milliseconds)
   *
   * Returns platform timestamp in milliseconds.
   * Overflow is acceptable (used for delta calculations).
   *
   * Example (ESP32-C6):
   *   return (uint32_t)(esp_timer_get_time() / 1000);
   *
   * Example (CH32V203):
   *   static volatile uint32_t tick = 0;
   *   // Increment tick in SysTick_Handler()
   *   return tick;
   *
   * @return Current tick count (ms)
   */
  uint32_t v4_task_platform_get_tick_ms(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
