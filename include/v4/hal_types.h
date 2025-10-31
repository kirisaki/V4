#ifndef V4_HAL_TYPES_H
#define V4_HAL_TYPES_H

/**
 * @file hal_types.h
 * @brief Type definitions for V4 HAL
 *
 * Provides C-compatible type definitions for GPIO, UART, and other peripherals.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /* ------------------------------------------------------------------------- */
  /* Opaque handle type                                                        */
  /* ------------------------------------------------------------------------- */

  /**
   * @brief Opaque handle for HAL resources
   *
   * Used for UART, SPI, I2C, and other peripheral handles.
   * Platform implementation determines the actual structure.
   */
  typedef void* hal_handle_t;

  /* ------------------------------------------------------------------------- */
  /* GPIO types                                                                */
  /* ------------------------------------------------------------------------- */

  /**
   * @brief GPIO pin mode
   */
  typedef enum
  {
    HAL_GPIO_INPUT = 0,      /**< High impedance input */
    HAL_GPIO_INPUT_PULLUP,   /**< Input with pull-up resistor */
    HAL_GPIO_INPUT_PULLDOWN, /**< Input with pull-down resistor */
    HAL_GPIO_OUTPUT,         /**< Push-pull output */
    HAL_GPIO_OUTPUT_OD,      /**< Open-drain output */
  } hal_gpio_mode_t;

  /**
   * @brief GPIO pin value
   */
  typedef enum
  {
    HAL_GPIO_LOW = 0,  /**< Logic low (0V) */
    HAL_GPIO_HIGH = 1, /**< Logic high (VCC) */
  } hal_gpio_value_t;

  /**
   * @brief GPIO interrupt edge type
   */
  typedef enum
  {
    HAL_GPIO_IRQ_RISING = 0x01,  /**< Rising edge trigger */
    HAL_GPIO_IRQ_FALLING = 0x02, /**< Falling edge trigger */
    HAL_GPIO_IRQ_BOTH = 0x03,    /**< Both edges trigger */
  } hal_gpio_irq_edge_t;

  /**
   * @brief GPIO interrupt handler callback
   *
   * @param pin       GPIO pin number that triggered
   * @param user_data User-provided context pointer
   */
  typedef void (*hal_gpio_irq_handler_t)(int pin, void* user_data);

  /* ------------------------------------------------------------------------- */
  /* UART types                                                                */
  /* ------------------------------------------------------------------------- */

  /**
   * @brief UART configuration structure
   */
  typedef struct
  {
    int baudrate;  /**< Baud rate (e.g., 9600, 115200) */
    int data_bits; /**< Data bits: 5, 6, 7, or 8 */
    int stop_bits; /**< Stop bits: 1 or 2 */
    int parity;    /**< Parity: 0=none, 1=odd, 2=even */
  } hal_uart_config_t;

#ifdef __cplusplus
}
#endif

#endif  // V4_HAL_TYPES_H
