# V4 Hardware Abstraction Layer (HAL) API

## Overview

The V4 HAL defines a minimal hardware abstraction layer for embedded systems. It provides a standard interface for:

- **GPIO**: Digital I/O control
- **UART**: Serial communication
- **Timer**: Time measurement and delays

V4-core **uses** these APIs via the `SYS` instruction. The actual implementation is provided by **V4-ports** for each target platform (CH32V, ESP32, RP2350, etc.).

## Design Principles

1. **Minimal API surface**: Only essential functionality
2. **No dynamic allocation**: All functions use stack or static storage
3. **Error codes**: All functions return `v4_err` (0 = success, negative = error)
4. **Platform-agnostic**: Pin numbers and port numbers are platform-specific
5. **C linkage**: `extern "C"` for compatibility

## API Reference

### GPIO API

#### `v4_hal_gpio_init`

```c
v4_err v4_hal_gpio_init(int pin, v4_hal_gpio_mode mode);
```

**Purpose**: Initialize a GPIO pin with specified mode.

**Parameters**:
- `pin`: GPIO pin number (platform-specific, e.g., `PA0`, `GPIO2`)
- `mode`: Pin mode (input, output, pull-up, pull-down)

**Modes**:
- `V4_HAL_GPIO_MODE_INPUT` (0): High impedance input
- `V4_HAL_GPIO_MODE_OUTPUT` (1): Push-pull output
- `V4_HAL_GPIO_MODE_INPUT_PULLUP` (2): Input with internal pull-up
- `V4_HAL_GPIO_MODE_INPUT_PULLDOWN` (3): Input with internal pull-down

**Returns**: 0 on success, negative error code on failure.

**Example**:
```c
// Configure PA0 as output
v4_hal_gpio_init(0, V4_HAL_GPIO_MODE_OUTPUT);
```

---

#### `v4_hal_gpio_write`

```c
v4_err v4_hal_gpio_write(int pin, int value);
```

**Purpose**: Write a digital value to a GPIO pin.

**Parameters**:
- `pin`: GPIO pin number
- `value`: 0 for LOW, non-zero for HIGH

**Returns**: 0 on success, negative error code on failure.

**Preconditions**: Pin must be initialized as output.

**Example**:
```c
// Set PA0 high
v4_hal_gpio_write(0, 1);

// Set PA0 low
v4_hal_gpio_write(0, 0);
```

---

#### `v4_hal_gpio_read`

```c
v4_err v4_hal_gpio_read(int pin, int *out_value);
```

**Purpose**: Read the current state of a GPIO pin.

**Parameters**:
- `pin`: GPIO pin number
- `out_value`: Pointer to store the read value (0 or 1)

**Returns**: 0 on success, negative error code on failure.

**Example**:
```c
int value;
if (v4_hal_gpio_read(0, &value) == 0) {
  // value is now 0 or 1
}
```

---

### UART API

#### `v4_hal_uart_init`

```c
v4_err v4_hal_uart_init(int port, int baudrate);
```

**Purpose**: Initialize a UART port with specified baud rate.

**Parameters**:
- `port`: UART port number (typically 0-3)
- `baudrate`: Baud rate (e.g., 9600, 115200)

**Format**: 8N1 (8 data bits, no parity, 1 stop bit)

**Returns**: 0 on success, negative error code on failure.

**Example**:
```c
// Initialize UART0 at 115200 baud
v4_hal_uart_init(0, 115200);
```

---

#### `v4_hal_uart_putc`

```c
v4_err v4_hal_uart_putc(int port, char c);
```

**Purpose**: Send a single character via UART.

**Parameters**:
- `port`: UART port number
- `c`: Character to send

**Behavior**: May block until transmit buffer has space.

**Returns**: 0 on success, negative error code on failure.

**Example**:
```c
v4_hal_uart_putc(0, 'A');
```

---

#### `v4_hal_uart_getc`

```c
v4_err v4_hal_uart_getc(int port, char *out_c);
```

**Purpose**: Receive a single character from UART.

**Parameters**:
- `port`: UART port number
- `out_c`: Pointer to store received character

**Behavior**: Non-blocking. Returns error if no data available.

**Returns**: 0 on success, negative error code if no data or error.

**Example**:
```c
char c;
if (v4_hal_uart_getc(0, &c) == 0) {
  // Character received
}
```

---

#### `v4_hal_uart_write`

```c
v4_err v4_hal_uart_write(int port, const char *buf, int len);
```

**Purpose**: Send a buffer via UART.

**Parameters**:
- `port`: UART port number
- `buf`: Pointer to data buffer
- `len`: Number of bytes to send

**Behavior**: May block until all data is sent.

**Returns**: 0 on success, negative error code on failure.

**Example**:
```c
const char *msg = "Hello";
v4_hal_uart_write(0, msg, 5);
```

---

#### `v4_hal_uart_read`

```c
v4_err v4_hal_uart_read(int port, char *buf, int max_len, int *out_len);
```

**Purpose**: Receive data from UART.

**Parameters**:
- `port`: UART port number
- `buf`: Pointer to destination buffer
- `max_len`: Maximum number of bytes to read
- `out_len`: Pointer to store actual bytes read

**Behavior**: Non-blocking. Returns immediately even if no data available.

**Returns**: 0 on success, negative error code on failure.

**Example**:
```c
char buf[64];
int received;
if (v4_hal_uart_read(0, buf, 64, &received) == 0) {
  // received bytes available in buf
}
```

---

### Timer API

#### `v4_hal_millis`

```c
uint32_t v4_hal_millis(void);
```

**Purpose**: Get milliseconds since system startup.

**Returns**: Milliseconds (wraps after ~49 days).

**Example**:
```c
uint32_t start = v4_hal_millis();
// ... do work ...
uint32_t elapsed = v4_hal_millis() - start;
```

---

#### `v4_hal_micros`

```c
uint64_t v4_hal_micros(void);
```

**Purpose**: Get microseconds since system startup.

**Returns**: Microseconds (64-bit, wraps after ~584,942 years).

**Example**:
```c
uint64_t start = v4_hal_micros();
// ... do work ...
uint64_t elapsed = v4_hal_micros() - start;
```

---

#### `v4_hal_delay_ms`

```c
void v4_hal_delay_ms(uint32_t ms);
```

**Purpose**: Blocking delay in milliseconds.

**Parameters**:
- `ms`: Milliseconds to delay

**Behavior**: Blocks execution. May use busy-wait or low-power sleep.

**Example**:
```c
v4_hal_delay_ms(1000);  // Wait 1 second
```

---

#### `v4_hal_delay_us`

```c
void v4_hal_delay_us(uint32_t us);
```

**Purpose**: Blocking delay in microseconds.

**Parameters**:
- `us`: Microseconds to delay

**Behavior**: Blocks execution. Typically uses busy-wait for accuracy.

**Example**:
```c
v4_hal_delay_us(100);  // Wait 100 microseconds
```

---

### System API (Optional)

#### `v4_hal_system_reset`

```c
void v4_hal_system_reset(void);
```

**Purpose**: Perform a system reset.

**Behavior**: Resets the microcontroller. May not return.

**Example**:
```c
v4_hal_system_reset();  // Reset MCU
```

---

#### `v4_hal_system_info`

```c
const char *v4_hal_system_info(void);
```

**Purpose**: Get platform identification string.

**Returns**: Pointer to static string (e.g., "CH32V307", "ESP32-C3").

**Example**:
```c
const char *info = v4_hal_system_info();
// info = "CH32V307"
```

---

## Implementation Guidelines

### For Port Developers (V4-ports)

When implementing HAL for a new platform:

1. **Include the header**:
   ```c
   #include "v4/v4_hal.h"
   ```

2. **Implement all required functions**:
   - GPIO: `gpio_init`, `gpio_write`, `gpio_read`
   - UART: `uart_init`, `uart_putc`, `uart_getc`
   - Timer: `millis`, `micros`, `delay_ms`, `delay_us`

3. **Use platform-specific APIs**:
   - Map V4 pin numbers to platform GPIO
   - Use vendor UART libraries
   - Implement timing with hardware timers

4. **Error handling**:
   - Return 0 on success
   - Return negative `v4_err` codes on failure (see `errors.def`)

5. **Example implementation** (CH32V307):
   ```c
   v4_err v4_hal_gpio_init(int pin, v4_hal_gpio_mode mode) {
     GPIO_InitTypeDef GPIO_InitStructure;
     // Map pin number to CH32 GPIO
     // Configure GPIO using CH32 HAL
     return 0;
   }
   ```

### For VM Users (V4-core)

The HAL is accessed via the `SYS` instruction in bytecode:

```forth
\ Set GPIO pin 0 high
0 1 SYS_GPIO_WRITE
```

See `docs/sys-opcodes.md` for SYS ID assignments.

---

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| CH32V307 | Planned | RISC-V, 144MHz |
| ESP32-C3 | Planned | RISC-V, WiFi/BLE |
| RP2350   | Planned | ARM Cortex-M33 + RISC-V |
| QEMU virt | Reference | Testing only |

---

## Error Codes

HAL functions return `v4_err` codes defined in `errors.def`:

| Code | Name | Description |
|------|------|-------------|
| 0 | OK | Success |
| -1 | InvalidArg | Invalid argument (pin, port, mode) |
| -2 | NotInitialized | HAL not initialized |
| -3 | Timeout | Operation timed out |
| -4 | Busy | Resource busy |
| -13 | OutOfBounds | Pin/port number out of range |

---

## Testing

### Mock HAL for Unit Tests

V4-core tests use a mock HAL implementation:

```c
// tests/mock_hal.cpp
v4_err v4_hal_gpio_write(int pin, int value) {
  // Record call for verification
  mock_gpio_log[pin] = value;
  return 0;
}
```

### Integration Tests

Full integration tests require real hardware or QEMU.

---

## References

- [V4 VM Architecture](../README.md)
- [SYS Instruction Specification](sys-opcodes.md)
- [Error Codes](../include/v4/errors.def)
