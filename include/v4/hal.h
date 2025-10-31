#ifndef V4_HAL_H
#define V4_HAL_H

/**
 * @file hal.h
 * @brief Unified Hardware Abstraction Layer API for V4 VM
 *
 * This header provides a complete HAL API including GPIO, UART, Timer,
 * and system control functions. It extends the basic v4_hal.h interface
 * with additional features like GPIO interrupts and UART handles.
 *
 * All functions use extern "C" linkage for C/C++ compatibility.
 */

#include "hal_capabilities.h"
#include "hal_error.h"
#include "hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /* ========================================================================= */
  /* Initialization and Control                                                */
  /* ========================================================================= */

  /**
   * @brief Initialize the HAL system
   *
   * Must be called before using any other HAL functions.
   * Performs platform-specific initialization.
   *
   * @return HAL_OK on success, negative error code on failure
   */
  int hal_init(void);

  /**
   * @brief Reset the HAL system
   *
   * Resets all HAL peripherals to their initial state.
   * Does not require hal_init() to be called again.
   *
   * @return HAL_OK on success, negative error code on failure
   */
  int hal_reset(void);

  /**
   * @brief Deinitialize the HAL system
   *
   * Releases all HAL resources. No HAL functions should be called
   * after this except hal_init().
   */
  void hal_deinit(void);

  /* ========================================================================= */
  /* GPIO API                                                                  */
  /* ========================================================================= */

  /**
   * @brief Configure GPIO pin mode
   *
   * @param pin  GPIO pin number (platform-specific)
   * @param mode Pin mode (input, output, pull-up, etc.)
   * @return HAL_OK on success, HAL_ERR_PARAM if pin invalid
   */
  int hal_gpio_mode(int pin, hal_gpio_mode_t mode);

  /**
   * @brief Write value to GPIO pin
   *
   * Pin must be configured as output.
   *
   * @param pin   GPIO pin number
   * @param value HAL_GPIO_LOW or HAL_GPIO_HIGH
   * @return HAL_OK on success, negative error code on failure
   */
  int hal_gpio_write(int pin, hal_gpio_value_t value);

  /**
   * @brief Read value from GPIO pin
   *
   * @param pin   GPIO pin number
   * @param value Pointer to store read value
   * @return HAL_OK on success, HAL_ERR_PARAM if value is NULL
   */
  int hal_gpio_read(int pin, hal_gpio_value_t* value);

  /**
   * @brief Toggle GPIO pin value
   *
   * Reads current pin state and writes the opposite value.
   * Pin must be configured as output.
   *
   * @param pin GPIO pin number
   * @return HAL_OK on success, negative error code on failure
   */
  int hal_gpio_toggle(int pin);

  /* ========================================================================= */
  /* GPIO Interrupt API (Tier 1)                                              */
  /* ========================================================================= */

  /**
   * @brief Attach interrupt handler to GPIO pin
   *
   * @param pin       GPIO pin number
   * @param edge      Interrupt edge type (rising, falling, both)
   * @param handler   Callback function
   * @param user_data User context passed to handler
   * @return HAL_OK on success, HAL_ERR_NOTSUP if interrupts not supported
   */
  int hal_gpio_irq_attach(int pin, hal_gpio_irq_edge_t edge,
                          hal_gpio_irq_handler_t handler, void* user_data);

  /**
   * @brief Detach interrupt handler from GPIO pin
   *
   * @param pin GPIO pin number
   * @return HAL_OK on success, negative error code on failure
   */
  int hal_gpio_irq_detach(int pin);

  /**
   * @brief Enable GPIO interrupt
   *
   * @param pin GPIO pin number
   * @return HAL_OK on success, negative error code on failure
   */
  int hal_gpio_irq_enable(int pin);

  /**
   * @brief Disable GPIO interrupt
   *
   * @param pin GPIO pin number
   * @return HAL_OK on success, negative error code on failure
   */
  int hal_gpio_irq_disable(int pin);

  /* ========================================================================= */
  /* UART API                                                                  */
  /* ========================================================================= */

  /**
   * @brief Open UART port
   *
   * @param port   UART port number (platform-specific, typically 0-3)
   * @param config Pointer to UART configuration
   * @return Opaque handle on success, NULL on failure
   */
  hal_handle_t hal_uart_open(int port, const hal_uart_config_t* config);

  /**
   * @brief Close UART port
   *
   * @param handle UART handle from hal_uart_open()
   * @return HAL_OK on success, HAL_ERR_PARAM if handle invalid
   */
  int hal_uart_close(hal_handle_t handle);

  /**
   * @brief Write data to UART
   *
   * Blocks until all data is written or timeout occurs.
   *
   * @param handle UART handle
   * @param buf    Data buffer
   * @param len    Number of bytes to write
   * @return Number of bytes written on success, negative error code on failure
   */
  int hal_uart_write(hal_handle_t handle, const uint8_t* buf, size_t len);

  /**
   * @brief Read data from UART
   *
   * Reads up to len bytes from UART receive buffer.
   * Returns immediately with available data (non-blocking).
   *
   * @param handle UART handle
   * @param buf    Destination buffer
   * @param len    Maximum bytes to read
   * @return Number of bytes read on success, negative error code on failure
   */
  int hal_uart_read(hal_handle_t handle, uint8_t* buf, size_t len);

  /**
   * @brief Get number of bytes available in UART receive buffer
   *
   * @param handle UART handle
   * @return Number of bytes available, negative error code on failure
   */
  int hal_uart_available(hal_handle_t handle);

  /* ========================================================================= */
  /* Timer API                                                                 */
  /* ========================================================================= */

  /**
   * @brief Get milliseconds since system startup
   *
   * Wraps around after approximately 49 days (32-bit counter).
   *
   * @return Milliseconds since startup
   */
  uint32_t hal_millis(void);

  /**
   * @brief Get microseconds since system startup
   *
   * 64-bit counter, wraps around after ~584,942 years.
   *
   * @return Microseconds since startup
   */
  uint64_t hal_micros(void);

  /**
   * @brief Blocking delay in milliseconds
   *
   * @param ms Milliseconds to delay
   */
  void hal_delay_ms(uint32_t ms);

  /**
   * @brief Blocking delay in microseconds
   *
   * @param us Microseconds to delay
   */
  void hal_delay_us(uint32_t us);

  /* ========================================================================= */
  /* Console I/O API                                                           */
  /* ========================================================================= */

  /**
   * @brief Write data to console output
   *
   * Writes bytes to the standard console output (typically stdout or UART0).
   * Blocks until all data is written.
   *
   * @param buf Data buffer
   * @param len Number of bytes to write
   * @return Number of bytes written on success, negative error code on failure
   */
  int hal_console_write(const uint8_t* buf, size_t len);

  /**
   * @brief Read data from console input
   *
   * Reads up to len bytes from the standard console input (typically stdin or UART0).
   * Blocking: waits until at least one byte is available.
   *
   * @param buf Destination buffer
   * @param len Maximum bytes to read
   * @return Number of bytes read on success, negative error code on failure
   */
  int hal_console_read(uint8_t* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif  // V4_HAL_H
