#pragma once

#include "v4/v4_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @file mock_hal.h
   * @brief Mock HAL control interface for unit testing
   *
   * Provides functions to control and inspect the mock HAL state.
   */

  /**
   * @brief Reset all mock HAL state
   *
   * Clears all GPIO pins, UART buffers, and timer counters.
   */
  void mock_hal_reset(void);

  /**
   * @brief Set the mock millisecond counter
   *
   * @param ms Milliseconds value
   */
  void mock_hal_set_millis(uint32_t ms);

  /**
   * @brief Set the mock microsecond counter
   *
   * @param us Microseconds value
   */
  void mock_hal_set_micros(uint64_t us);

  /**
   * @brief Inject data into UART receive buffer
   *
   * @param port UART port number
   * @param data Data to inject
   * @param len Length of data
   */
  void mock_hal_uart_inject_rx(int port, const char* data, int len);

  /**
   * @brief Get transmitted UART data
   *
   * @param port    UART port number
   * @param out_len Pointer to store data length (can be NULL)
   * @return Pointer to TX buffer
   */
  const char* mock_hal_uart_get_tx(int port, int* out_len);

  /**
   * @brief Get GPIO pin value
   *
   * @param pin GPIO pin number
   * @return Pin value (0 or 1), or -1 on error
   */
  int mock_hal_gpio_get_value(int pin);

  /**
   * @brief Get GPIO pin mode
   *
   * @param pin GPIO pin number
   * @return Pin mode
   */
  v4_hal_gpio_mode mock_hal_gpio_get_mode(int pin);

#ifdef __cplusplus
}
#endif
