#ifndef V4_HAL_CAPABILITIES_H
#define V4_HAL_CAPABILITIES_H

/**
 * @file hal_capabilities.h
 * @brief Platform capability discovery for V4 HAL
 *
 * Allows runtime querying of platform features and resource limits.
 * Each platform implementation provides a hal_platform_capabilities()
 * function as a weak symbol.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Platform capability structure
   *
   * Describes available hardware resources and features.
   * All counts represent maximum available resources (e.g., gpio_count
   * indicates highest valid GPIO pin number + 1).
   */
  typedef struct
  {
    uint8_t gpio_count; /**< Number of GPIO pins */
    uint8_t uart_count; /**< Number of UART ports */
    uint8_t spi_count;  /**< Number of SPI buses */
    uint8_t i2c_count;  /**< Number of I2C buses */

    /** Feature flags (bitfield) */
    uint8_t has_adc : 1;  /**< ADC available */
    uint8_t has_dac : 1;  /**< DAC available */
    uint8_t has_pwm : 1;  /**< PWM available */
    uint8_t has_rtc : 1;  /**< RTC available */
    uint8_t has_dma : 1;  /**< DMA available */
    uint8_t reserved : 3; /**< Reserved for future use */
  } hal_capabilities_t;

  /**
   * @brief Get platform capabilities
   *
   * Returns a pointer to a static capability structure describing
   * the current platform's hardware resources.
   *
   * @return Pointer to static hal_capabilities_t structure (never NULL)
   */
  const hal_capabilities_t* hal_get_capabilities(void);

#ifdef __cplusplus
}
#endif

#endif  // V4_HAL_CAPABILITIES_H
