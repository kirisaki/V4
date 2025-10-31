#pragma once

#include <stdint.h>

#include "v4/vm_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @file v4_hal.h
   * @brief Hardware Abstraction Layer (HAL) for V4 VM
   *
   * This header defines the HAL interface that port implementations must provide.
   * V4-core uses these functions via the SYS instruction to interact with hardware.
   *
   * **Implementation Responsibility**: V4-ports (not V4-core)
   *
   * All HAL functions return v4_err (0 = success, negative = error).
   * Error codes are defined in errors.def.
   */

  /* ------------------------------------------------------------------------- */
  /* GPIO API                                                                  */
  /* ------------------------------------------------------------------------- */

  /**
   * @brief GPIO pin mode configuration
   */
  typedef enum
  {
    V4_HAL_GPIO_MODE_INPUT = 0,         /**< Input mode (high impedance) */
    V4_HAL_GPIO_MODE_OUTPUT = 1,        /**< Output mode (push-pull) */
    V4_HAL_GPIO_MODE_INPUT_PULLUP = 2,  /**< Input with pull-up resistor */
    V4_HAL_GPIO_MODE_INPUT_PULLDOWN = 3 /**< Input with pull-down resistor */
  } v4_hal_gpio_mode;

  /**
   * @brief Initialize a GPIO pin
   *
   * Configures the specified pin to the given mode.
   * Pin numbers are platform-specific.
   *
   * @param pin  GPIO pin number (platform-specific)
   * @param mode Pin mode (input, output, pull-up, pull-down)
   * @return 0 on success, negative error code on failure
   */
  v4_err v4_hal_gpio_init(int pin, v4_hal_gpio_mode mode);

  /**
   * @brief Write a value to a GPIO pin
   *
   * Sets the pin to high (1) or low (0).
   * The pin must be configured as output.
   *
   * @param pin   GPIO pin number
   * @param value 0 for LOW, non-zero for HIGH
   * @return 0 on success, negative error code on failure
   */
  v4_err v4_hal_gpio_write(int pin, int value);

  /**
   * @brief Read a value from a GPIO pin
   *
   * Reads the current state of the pin.
   * The pin should be configured as input.
   *
   * @param pin       GPIO pin number
   * @param out_value Pointer to store the read value (0 or 1)
   * @return 0 on success, negative error code on failure
   */
  v4_err v4_hal_gpio_read(int pin, int *out_value);

  /* ------------------------------------------------------------------------- */
  /* UART API                                                                  */
  /* ------------------------------------------------------------------------- */

  /**
   * @brief Initialize a UART port
   *
   * Configures the specified UART port with the given baud rate.
   * Uses 8N1 format (8 data bits, no parity, 1 stop bit) by default.
   *
   * @param port     UART port number (platform-specific, typically 0-3)
   * @param baudrate Baud rate (e.g., 9600, 115200)
   * @return 0 on success, negative error code on failure
   */
  v4_err v4_hal_uart_init(int port, int baudrate);

  /**
   * @brief Send a single character via UART
   *
   * Transmits one byte. May block until the transmit buffer has space.
   *
   * @param port UART port number
   * @param c    Character to send
   * @return 0 on success, negative error code on failure
   */
  v4_err v4_hal_uart_putc(int port, char c);

  /**
   * @brief Receive a single character from UART
   *
   * Reads one byte from the receive buffer.
   * Returns an error if no data is available (non-blocking).
   *
   * @param port  UART port number
   * @param out_c Pointer to store the received character
   * @return 0 on success, negative error code if no data or error
   */
  v4_err v4_hal_uart_getc(int port, char *out_c);

  /**
   * @brief Send a buffer via UART
   *
   * Transmits multiple bytes. May block until all data is sent.
   *
   * @param port UART port number
   * @param buf  Pointer to data buffer
   * @param len  Number of bytes to send
   * @return 0 on success, negative error code on failure
   */
  v4_err v4_hal_uart_write(int port, const char *buf, int len);

  /**
   * @brief Receive data from UART
   *
   * Reads up to max_len bytes from the receive buffer.
   * Returns the actual number of bytes read in *out_len.
   * Non-blocking: returns immediately even if no data available.
   *
   * @param port    UART port number
   * @param buf     Pointer to destination buffer
   * @param max_len Maximum number of bytes to read
   * @param out_len Pointer to store actual bytes read
   * @return 0 on success, negative error code on failure
   */
  v4_err v4_hal_uart_read(int port, char *buf, int max_len, int *out_len);

  /* ------------------------------------------------------------------------- */
  /* Timer API                                                                 */
  /* ------------------------------------------------------------------------- */

  /**
   * @brief Get milliseconds since system startup
   *
   * Returns the number of milliseconds elapsed since the system started.
   * Wraps around after ~49 days (32-bit counter).
   *
   * @return Milliseconds since startup
   */
  uint32_t v4_hal_millis(void);

  /**
   * @brief Get microseconds since system startup
   *
   * Returns the number of microseconds elapsed since the system started.
   * Wraps around after ~1.19 hours (32-bit counter) or ~584,942 years (64-bit).
   *
   * @return Microseconds since startup
   */
  uint64_t v4_hal_micros(void);

  /**
   * @brief Blocking delay in milliseconds
   *
   * Blocks execution for the specified number of milliseconds.
   * Uses busy-wait or low-power sleep depending on implementation.
   *
   * @param ms Milliseconds to delay
   */
  void v4_hal_delay_ms(uint32_t ms);

  /**
   * @brief Blocking delay in microseconds
   *
   * Blocks execution for the specified number of microseconds.
   * Typically uses busy-wait for accurate short delays.
   *
   * @param us Microseconds to delay
   */
  void v4_hal_delay_us(uint32_t us);

  /* ------------------------------------------------------------------------- */
  /* Console I/O API                                                           */
  /* ------------------------------------------------------------------------- */

  /**
   * @brief Output one character to the console
   *
   * Writes a single character to the standard output (console).
   * This is typically used for interactive REPL or debug output.
   * May block until the character is transmitted.
   *
   * @param c Character to output
   * @return 0 on success, negative error code on failure
   */
  v4_err v4_hal_putc(char c);

  /**
   * @brief Input one character from the console
   *
   * Reads a single character from the standard input (console).
   * This is blocking: waits until a character is available.
   * Used for interactive REPL input.
   *
   * @param out_c Pointer to store the read character
   * @return 0 on success, negative error code on failure
   */
  v4_err v4_hal_getc(char *out_c);

  /* ------------------------------------------------------------------------- */
  /* System API (optional)                                                     */
  /* ------------------------------------------------------------------------- */

  /**
   * @brief Perform system reset (optional)
   *
   * Resets the microcontroller. Implementation-specific.
   * May not return.
   */
  void v4_hal_system_reset(void);

  /**
   * @brief Get system information (optional)
   *
   * Returns a platform-specific system information string.
   * Useful for debugging and identification.
   *
   * @return Pointer to static string (e.g., "CH32V307", "ESP32-C3")
   */
  const char *v4_hal_system_info(void);

#ifdef __cplusplus
}
#endif
