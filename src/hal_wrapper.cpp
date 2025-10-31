/**
 * @file hal_wrapper.cpp
 * @brief Bridge layer between V4's v4_hal_* API and V4-hal's hal_* API
 *
 * This wrapper allows V4 to use the C++17 CRTP implementation from V4-hal
 * while maintaining backward compatibility with the existing v4_hal_* API.
 */

#include <cstring>

#include "v4/hal.h"
#include "v4/v4_hal.h"

/* ========================================================================= */
/* GPIO API Mapping                                                          */
/* ========================================================================= */

extern "C" v4_err v4_hal_gpio_init(int pin, v4_hal_gpio_mode mode)
{
  hal_gpio_mode_t hal_mode;

  // Map v4_hal_gpio_mode to hal_gpio_mode_t
  switch (mode)
  {
    case V4_HAL_GPIO_MODE_INPUT:
      hal_mode = HAL_GPIO_INPUT;
      break;
    case V4_HAL_GPIO_MODE_OUTPUT:
      hal_mode = HAL_GPIO_OUTPUT;
      break;
    case V4_HAL_GPIO_MODE_INPUT_PULLUP:
      hal_mode = HAL_GPIO_INPUT_PULLUP;
      break;
    case V4_HAL_GPIO_MODE_INPUT_PULLDOWN:
      hal_mode = HAL_GPIO_INPUT_PULLDOWN;
      break;
    default:
      return -1;  // HAL_ERR_PARAM
  }

  return hal_gpio_mode(pin, hal_mode);
}

extern "C" v4_err v4_hal_gpio_write(int pin, int value)
{
  hal_gpio_value_t hal_value = value ? HAL_GPIO_HIGH : HAL_GPIO_LOW;
  return hal_gpio_write(pin, hal_value);
}

extern "C" v4_err v4_hal_gpio_read(int pin, int* out_value)
{
  if (!out_value)
  {
    return -1;  // HAL_ERR_PARAM
  }

  hal_gpio_value_t hal_value;
  int ret = hal_gpio_read(pin, &hal_value);
  if (ret == 0)
  {
    *out_value = (hal_value == HAL_GPIO_HIGH) ? 1 : 0;
  }
  return ret;
}

/* ========================================================================= */
/* UART API Mapping                                                          */
/* ========================================================================= */

// Simple UART port to handle mapping (V4-hal uses handles, V4 uses port numbers)
static hal_handle_t uart_handles[4] = {nullptr, nullptr, nullptr, nullptr};

extern "C" v4_err v4_hal_uart_init(int port, int baudrate)
{
  if (port < 0 || port >= 4)
  {
    return -1;  // HAL_ERR_PARAM
  }

  // Close existing handle if open
  if (uart_handles[port])
  {
    hal_uart_close(uart_handles[port]);
    uart_handles[port] = nullptr;
  }

  // Configure UART (8N1 format)
  hal_uart_config_t config;
  config.baudrate = baudrate;
  config.data_bits = 8;
  config.parity = 0;     // 0=none, 1=odd, 2=even
  config.stop_bits = 1;  // 1 or 2

  uart_handles[port] = hal_uart_open(port, &config);
  return uart_handles[port] ? 0 : -4;  // HAL_OK or HAL_ERR_NODEV
}

extern "C" v4_err v4_hal_uart_putc(int port, char c)
{
  if (port < 0 || port >= 4 || !uart_handles[port])
  {
    return -1;  // HAL_ERR_PARAM or HAL_ERR_NODEV
  }

  uint8_t byte = static_cast<uint8_t>(c);
  int ret = hal_uart_write(uart_handles[port], &byte, 1);
  return (ret == 1) ? 0 : -1;
}

extern "C" v4_err v4_hal_uart_getc(int port, char* out_c)
{
  if (port < 0 || port >= 4 || !uart_handles[port] || !out_c)
  {
    return -1;  // HAL_ERR_PARAM or HAL_ERR_NODEV
  }

  uint8_t byte;
  int ret = hal_uart_read(uart_handles[port], &byte, 1);
  if (ret == 1)
  {
    *out_c = static_cast<char>(byte);
    return 0;
  }
  return -1;  // No data available or error
}

extern "C" v4_err v4_hal_uart_write(int port, const char* buf, int len)
{
  if (port < 0 || port >= 4 || !uart_handles[port] || !buf || len < 0)
  {
    return -1;  // HAL_ERR_PARAM
  }

  int ret = hal_uart_write(uart_handles[port], reinterpret_cast<const uint8_t*>(buf),
                           static_cast<size_t>(len));
  return (ret == len) ? 0 : -1;
}

extern "C" v4_err v4_hal_uart_read(int port, char* buf, int max_len, int* out_len)
{
  if (port < 0 || port >= 4 || !uart_handles[port] || !buf || max_len < 0 || !out_len)
  {
    return -1;  // HAL_ERR_PARAM
  }

  int ret = hal_uart_read(uart_handles[port], reinterpret_cast<uint8_t*>(buf),
                          static_cast<size_t>(max_len));
  if (ret >= 0)
  {
    *out_len = ret;
    return 0;
  }
  return -1;
}

/* ========================================================================= */
/* Timer API Mapping                                                         */
/* ========================================================================= */

extern "C" uint32_t v4_hal_millis(void)
{
  return hal_millis();
}

extern "C" uint64_t v4_hal_micros(void)
{
  return hal_micros();
}

extern "C" void v4_hal_delay_ms(uint32_t ms)
{
  hal_delay_ms(ms);
}

extern "C" void v4_hal_delay_us(uint32_t us)
{
  hal_delay_us(us);
}

/* ========================================================================= */
/* Console I/O API Mapping                                                   */
/* ========================================================================= */

extern "C" v4_err v4_hal_putc(char c)
{
  uint8_t byte = static_cast<uint8_t>(c);
  int ret = hal_console_write(&byte, 1);
  return (ret == 1) ? 0 : -1;
}

extern "C" v4_err v4_hal_getc(char* out_c)
{
  if (!out_c)
  {
    return -1;  // HAL_ERR_PARAM
  }

  uint8_t byte;
  int ret = hal_console_read(&byte, 1);
  if (ret == 1)
  {
    *out_c = static_cast<char>(byte);
    return 0;
  }
  return -1;  // No data available or error
}

/* ========================================================================= */
/* System API Mapping                                                        */
/* ========================================================================= */

extern "C" void v4_hal_system_reset(void)
{
  // V4-hal doesn't have system_reset in hal.h yet
  // For now, call hal_deinit and hal_init
  hal_deinit();
  hal_init();
}

extern "C" const char* v4_hal_system_info(void)
{
  // V4-hal doesn't have system_info
  // Return a simple string for now
  static const char info[] = "V4-hal C++17 CRTP implementation (POSIX)";
  return info;
}
