#include <cstring>

#include "v4/hal.h"

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
#define CONSOLE_BUFFER_SIZE 256

struct MockGpioState
{
  int initialized;
  hal_gpio_mode_t mode;
  hal_gpio_value_t value;
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

struct MockConsoleState
{
  char output_buffer[CONSOLE_BUFFER_SIZE];
  int output_count;
  char input_buffer[CONSOLE_BUFFER_SIZE];
  int input_count;
  int input_pos;
};

static struct MockGpioState mock_gpio[MAX_GPIO_PINS];
static struct MockUartState mock_uart[MAX_UART_PORTS];
static struct MockConsoleState mock_console;
static uint32_t mock_millis_counter = 0;
static uint64_t mock_micros_counter = 0;

/* ------------------------------------------------------------------------- */
/* Mock control functions (for tests)                                       */
/* ------------------------------------------------------------------------- */

extern "C" void mock_hal_reset(void)
{
  memset(mock_gpio, 0, sizeof(mock_gpio));
  memset(mock_uart, 0, sizeof(mock_uart));
  memset(&mock_console, 0, sizeof(mock_console));
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

extern "C" hal_gpio_value_t mock_hal_gpio_get_value(int pin)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return HAL_GPIO_LOW;
  return mock_gpio[pin].value;
}

extern "C" hal_gpio_mode_t mock_hal_gpio_get_mode(int pin)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return HAL_GPIO_INPUT;
  return mock_gpio[pin].mode;
}

extern "C" void mock_hal_console_inject_input(const char* data, int len)
{
  if (len > CONSOLE_BUFFER_SIZE)
    len = CONSOLE_BUFFER_SIZE;

  memcpy(mock_console.input_buffer, data, len);
  mock_console.input_count = len;
  mock_console.input_pos = 0;
}

extern "C" const char* mock_hal_console_get_output(int* out_len)
{
  if (out_len)
    *out_len = mock_console.output_count;
  return mock_console.output_buffer;
}

/* ------------------------------------------------------------------------- */
/* GPIO API                                                                  */
/* ------------------------------------------------------------------------- */

extern "C" int hal_gpio_mode(int pin, hal_gpio_mode_t mode)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return HAL_ERR_PARAM;

  mock_gpio[pin].initialized = 1;
  mock_gpio[pin].mode = mode;
  mock_gpio[pin].value = HAL_GPIO_LOW;
  return HAL_OK;
}

extern "C" int hal_gpio_write(int pin, hal_gpio_value_t value)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return HAL_ERR_PARAM;

  if (!mock_gpio[pin].initialized)
    return HAL_ERR_PARAM;

  if (mock_gpio[pin].mode != HAL_GPIO_OUTPUT && mock_gpio[pin].mode != HAL_GPIO_OUTPUT_OD)
    return HAL_ERR_PARAM;

  mock_gpio[pin].value = value;
  return HAL_OK;
}

extern "C" int hal_gpio_read(int pin, hal_gpio_value_t* out_value)
{
  if (pin < 0 || pin >= MAX_GPIO_PINS)
    return HAL_ERR_PARAM;

  if (!mock_gpio[pin].initialized)
    return HAL_ERR_PARAM;

  if (!out_value)
    return HAL_ERR_PARAM;

  *out_value = mock_gpio[pin].value;
  return HAL_OK;
}

/* ------------------------------------------------------------------------- */
/* UART API                                                                  */
/* ------------------------------------------------------------------------- */

extern "C" hal_handle_t hal_uart_open(int port, const hal_uart_config_t* config)
{
  if (port < 0 || port >= MAX_UART_PORTS)
    return nullptr;

  if (!config || config->baudrate <= 0)
    return nullptr;

  mock_uart[port].initialized = 1;
  mock_uart[port].baudrate = config->baudrate;
  mock_uart[port].tx_count = 0;
  mock_uart[port].rx_count = 0;
  mock_uart[port].rx_pos = 0;

  return &mock_uart[port];
}

extern "C" int hal_uart_close(hal_handle_t handle)
{
  if (!handle)
    return HAL_ERR_PARAM;

  MockUartState* uart = static_cast<MockUartState*>(handle);
  uart->initialized = 0;
  return HAL_OK;
}

extern "C" int hal_uart_write(hal_handle_t handle, const uint8_t* buf, size_t len)
{
  if (!handle || !buf)
    return HAL_ERR_PARAM;

  MockUartState* uart = static_cast<MockUartState*>(handle);

  if (!uart->initialized)
    return HAL_ERR_PARAM;

  size_t written = 0;
  for (size_t i = 0; i < len; i++)
  {
    if (uart->tx_count >= UART_BUFFER_SIZE)
      break;

    uart->tx_buffer[uart->tx_count++] = buf[i];
    written++;
  }

  return written;
}

extern "C" int hal_uart_read(hal_handle_t handle, uint8_t* buf, size_t len)
{
  if (!handle || !buf)
    return HAL_ERR_PARAM;

  MockUartState* uart = static_cast<MockUartState*>(handle);

  if (!uart->initialized)
    return HAL_ERR_PARAM;

  int available = uart->rx_count - uart->rx_pos;
  size_t to_read = (available < (int)len) ? available : len;

  memcpy(buf, uart->rx_buffer + uart->rx_pos, to_read);
  uart->rx_pos += to_read;

  return to_read;
}

extern "C" int hal_uart_available(hal_handle_t handle)
{
  if (!handle)
    return HAL_ERR_PARAM;

  MockUartState* uart = static_cast<MockUartState*>(handle);

  if (!uart->initialized)
    return HAL_ERR_PARAM;

  return uart->rx_count - uart->rx_pos;
}

/* ------------------------------------------------------------------------- */
/* Timer API                                                                 */
/* ------------------------------------------------------------------------- */

extern "C" uint32_t hal_millis(void)
{
  return mock_millis_counter;
}

extern "C" uint64_t hal_micros(void)
{
  return mock_micros_counter;
}

extern "C" void hal_delay_ms(uint32_t ms)
{
  // Mock delay: just advance counter
  mock_millis_counter += ms;
  mock_micros_counter += ms * 1000ULL;
}

extern "C" void hal_delay_us(uint32_t us)
{
  // Mock delay: just advance counter
  mock_micros_counter += us;
  mock_millis_counter += us / 1000;
}

/* ------------------------------------------------------------------------- */
/* Console I/O API                                                           */
/* ------------------------------------------------------------------------- */

extern "C" int hal_console_write(const uint8_t* buf, size_t len)
{
  if (!buf)
    return HAL_ERR_PARAM;

  size_t written = 0;
  for (size_t i = 0; i < len; i++)
  {
    if (mock_console.output_count >= CONSOLE_BUFFER_SIZE)
      break;

    mock_console.output_buffer[mock_console.output_count++] = buf[i];
    written++;
  }

  return written;
}

extern "C" int hal_console_read(uint8_t* buf, size_t len)
{
  if (!buf)
    return HAL_ERR_PARAM;

  size_t read_count = 0;
  while (read_count < len && mock_console.input_pos < mock_console.input_count)
  {
    buf[read_count++] = mock_console.input_buffer[mock_console.input_pos++];
  }

  return read_count;
}
