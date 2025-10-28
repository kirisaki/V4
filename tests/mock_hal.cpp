#include <cstring>

#include "v4/v4_hal.h"

/**
 * @file mock_hal.cpp
 * @brief Mock HAL implementation for unit testing
 *
 * Provides simple recording/playback functionality for testing
 * SYS instruction without real hardware.
 */

/* ------------------------------------------------------------------------- */
/* Mock state tracking                                                       */
/* ------------------------------------------------------------------------- */

#define MAX_GPIO_PINS 32
#define MAX_UART_PORTS 4
#define UART_BUFFER_SIZE 256

struct MockGpioState
{
  int initialized;
  v4_hal_gpio_mode mode;
  int value;
};

struct MockUartState
{
  int initialized;
  int baudrate;
  char tx_buffer[UART_BUFFER_SIZE];
  int tx_count;
  char rx_buffer[UART_BUFFER_SIZE];
  int rx_count;
  int rx_pos;
};

static struct MockGpioState mock_gpio[MAX_GPIO_PINS];
static struct MockUartState mock_uart[MAX_UART_PORTS];
static uint32_t mock_millis_counter = 0;
static uint64_t mock_micros_counter = 0;

/* ------------------------------------------------------------------------- */
/* Mock control functions (for tests)                                       */
/* ------------------------------------------------------------------------- */

extern "C" void mock_hal_reset(void)
{
  memset(mock_gpio, 0, sizeof(mock_gpio));
  memset(mock_uart, 0, sizeof(mock_uart));
  mock_millis_counter = 0;
  mock_micros_counter = 0;
}

extern "C" void mock_hal_set_millis(uint32_t ms)
{
  mock_millis_counter = ms;
}

extern "C" void mock_hal_set_micros(uint64_t us)
{
  mock_micros_counter = us;
}

extern "C" void mock_hal_uart_inject_rx(int port, const char* data, int len)
{
  if (port < 0 || port >= MAX_UART_PORTS)
    return;
  if (len > UART_BUFFER_SIZE)
    len = UART_BUFFER_SIZE;

  memcpy(mock_uart[port].rx_buffer, data, len);
  mock_uart[port].rx_count = len;
  mock_uart[port].rx_pos = 0;
}

extern "C" const char* mock_hal_uart_get_tx(int port, int* out_len)
{
  if (port < 0 || port >= MAX_UART_PORTS)
    return nullptr;

  if (out_len)
    *out_len = mock_uart[port].tx_count;
  return mock_uart[port].tx_buffer;
}

extern "C" int mock_hal_gpio_get_value(int pin)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return -1;
  return mock_gpio[pin].value;
}

extern "C" v4_hal_gpio_mode mock_hal_gpio_get_mode(int pin)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return V4_HAL_GPIO_MODE_INPUT;
  return mock_gpio[pin].mode;
}

/* ------------------------------------------------------------------------- */
/* GPIO API                                                                  */
/* ------------------------------------------------------------------------- */

extern "C" v4_err v4_hal_gpio_init(int pin, v4_hal_gpio_mode mode)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return -13;  // OutOfBounds

  mock_gpio[pin].initialized = 1;
  mock_gpio[pin].mode = mode;
  mock_gpio[pin].value = 0;
  return 0;
}

extern "C" v4_err v4_hal_gpio_write(int pin, int value)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return -13;  // OutOfBounds

  if (!mock_gpio[pin].initialized)
    return -2;  // NotInitialized

  if (mock_gpio[pin].mode != V4_HAL_GPIO_MODE_OUTPUT)
    return -1;  // InvalidArg

  mock_gpio[pin].value = value ? 1 : 0;
  return 0;
}

extern "C" v4_err v4_hal_gpio_read(int pin, int* out_value)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return -13;  // OutOfBounds

  if (!mock_gpio[pin].initialized)
    return -2;  // NotInitialized

  if (!out_value)
    return -1;  // InvalidArg

  *out_value = mock_gpio[pin].value;
  return 0;
}

/* ------------------------------------------------------------------------- */
/* UART API                                                                  */
/* ------------------------------------------------------------------------- */

extern "C" v4_err v4_hal_uart_init(int port, int baudrate)
{
  if (port < 0 || port >= MAX_UART_PORTS)
    return -13;  // OutOfBounds

  if (baudrate <= 0)
    return -1;  // InvalidArg

  mock_uart[port].initialized = 1;
  mock_uart[port].baudrate = baudrate;
  mock_uart[port].tx_count = 0;
  mock_uart[port].rx_count = 0;
  mock_uart[port].rx_pos = 0;
  return 0;
}

extern "C" v4_err v4_hal_uart_putc(int port, char c)
{
  if (port < 0 || port >= MAX_UART_PORTS)
    return -13;  // OutOfBounds

  if (!mock_uart[port].initialized)
    return -2;  // NotInitialized

  if (mock_uart[port].tx_count >= UART_BUFFER_SIZE)
    return -4;  // Busy

  mock_uart[port].tx_buffer[mock_uart[port].tx_count++] = c;
  return 0;
}

extern "C" v4_err v4_hal_uart_getc(int port, char* out_c)
{
  if (port < 0 || port >= MAX_UART_PORTS)
    return -13;  // OutOfBounds

  if (!mock_uart[port].initialized)
    return -2;  // NotInitialized

  if (!out_c)
    return -1;  // InvalidArg

  if (mock_uart[port].rx_pos >= mock_uart[port].rx_count)
    return -3;  // Timeout (no data)

  *out_c = mock_uart[port].rx_buffer[mock_uart[port].rx_pos++];
  return 0;
}

extern "C" v4_err v4_hal_uart_write(int port, const char* buf, int len)
{
  if (port < 0 || port >= MAX_UART_PORTS)
    return -13;  // OutOfBounds

  if (!mock_uart[port].initialized)
    return -2;  // NotInitialized

  if (!buf || len < 0)
    return -1;  // InvalidArg

  for (int i = 0; i < len; i++)
  {
    if (mock_uart[port].tx_count >= UART_BUFFER_SIZE)
      return -4;  // Busy

    mock_uart[port].tx_buffer[mock_uart[port].tx_count++] = buf[i];
  }

  return 0;
}

extern "C" v4_err v4_hal_uart_read(int port, char* buf, int max_len, int* out_len)
{
  if (port < 0 || port >= MAX_UART_PORTS)
    return -13;  // OutOfBounds

  if (!mock_uart[port].initialized)
    return -2;  // NotInitialized

  if (!buf || !out_len || max_len < 0)
    return -1;  // InvalidArg

  int available = mock_uart[port].rx_count - mock_uart[port].rx_pos;
  int to_read = (available < max_len) ? available : max_len;

  memcpy(buf, mock_uart[port].rx_buffer + mock_uart[port].rx_pos, to_read);
  mock_uart[port].rx_pos += to_read;
  *out_len = to_read;

  return 0;
}

/* ------------------------------------------------------------------------- */
/* Timer API                                                                 */
/* ------------------------------------------------------------------------- */

extern "C" uint32_t v4_hal_millis(void)
{
  return mock_millis_counter;
}

extern "C" uint64_t v4_hal_micros(void)
{
  return mock_micros_counter;
}

extern "C" void v4_hal_delay_ms(uint32_t ms)
{
  // Mock delay: just advance counter
  mock_millis_counter += ms;
  mock_micros_counter += ms * 1000ULL;
}

extern "C" void v4_hal_delay_us(uint32_t us)
{
  // Mock delay: just advance counter
  mock_micros_counter += us;
  mock_millis_counter += us / 1000;
}

/* ------------------------------------------------------------------------- */
/* System API                                                                */
/* ------------------------------------------------------------------------- */

extern "C" void v4_hal_system_reset(void)
{
  // Mock reset: clear all state
  mock_hal_reset();
}

extern "C" const char* v4_hal_system_info(void)
{
  return "Mock HAL v1.0";
}
