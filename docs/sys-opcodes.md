# V4 SYS Instruction Opcodes

## Overview

The `SYS` instruction (opcode `0x60`) provides a gateway to Hardware Abstraction Layer (HAL) functions. It allows V4 bytecode to interact with GPIO, UART, and timers without platform-specific knowledge.

**Format**: `SYS <id:u8>`

Where `id` specifies which HAL function to call.

## SYS ID Assignment

### GPIO Operations (0x00 - 0x0F)

| ID   | Function | Stack Effect | Description |
|------|----------|--------------|-------------|
| 0x00 | `GPIO_INIT` | `(pin mode -- err)` | Initialize GPIO pin |
| 0x01 | `GPIO_WRITE` | `(pin value -- err)` | Write to GPIO pin |
| 0x02 | `GPIO_READ` | `(pin -- value err)` | Read from GPIO pin |

#### GPIO_INIT (0x00)

**Stack**: `(pin mode -- err)`

**Description**: Initialize a GPIO pin with specified mode.

**Parameters**:
- `pin`: GPIO pin number (platform-specific)
- `mode`: Pin mode (0=INPUT, 1=OUTPUT, 2=INPUT_PULLUP, 3=INPUT_PULLDOWN)

**Returns**:
- `err`: Error code (0 = success)

**Example**:
```forth
\ Configure pin 13 as output
13 1 SYS 0x00
```

---

#### GPIO_WRITE (0x01)

**Stack**: `(pin value -- err)`

**Description**: Write a digital value to GPIO pin.

**Parameters**:
- `pin`: GPIO pin number
- `value`: 0 for LOW, non-zero for HIGH

**Returns**:
- `err`: Error code (0 = success)

**Example**:
```forth
\ Set pin 13 high
13 1 SYS 0x01

\ Set pin 13 low
13 0 SYS 0x01
```

---

#### GPIO_READ (0x02)

**Stack**: `(pin -- value err)`

**Description**: Read the current state of GPIO pin.

**Parameters**:
- `pin`: GPIO pin number

**Returns**:
- `value`: Pin state (0 or 1)
- `err`: Error code (0 = success)

**Example**:
```forth
\ Read pin 5
5 SYS 0x02
\ Stack now has: value err
```

---

### UART Operations (0x10 - 0x1F)

| ID   | Function | Stack Effect | Description |
|------|----------|--------------|-------------|
| 0x10 | `UART_INIT` | `(port baudrate -- err)` | Initialize UART port |
| 0x11 | `UART_PUTC` | `(port char -- err)` | Send one character |
| 0x12 | `UART_GETC` | `(port -- char err)` | Receive one character |

#### UART_INIT (0x10)

**Stack**: `(port baudrate -- err)`

**Description**: Initialize UART port with specified baud rate.

**Parameters**:
- `port`: UART port number (typically 0-3)
- `baudrate`: Baud rate (e.g., 9600, 115200)

**Returns**:
- `err`: Error code (0 = success)

**Example**:
```forth
\ Initialize UART0 at 115200 baud
0 115200 SYS 0x10
```

---

#### UART_PUTC (0x11)

**Stack**: `(port char -- err)`

**Description**: Send a single character via UART.

**Parameters**:
- `port`: UART port number
- `char`: Character to send (ASCII value)

**Returns**:
- `err`: Error code (0 = success)

**Example**:
```forth
\ Send 'A' (ASCII 65) to UART0
0 65 SYS 0x11
```

---

#### UART_GETC (0x12)

**Stack**: `(port -- char err)`

**Description**: Receive a single character from UART (non-blocking).

**Parameters**:
- `port`: UART port number

**Returns**:
- `char`: Received character (ASCII value), or undefined if error
- `err`: Error code (0 = success, negative = no data or error)

**Example**:
```forth
\ Try to receive from UART0
0 SYS 0x12
\ Stack: char err
\ Check if err is 0 before using char
```

---

### Timer Operations (0x20 - 0x2F)

| ID   | Function | Stack Effect | Description |
|------|----------|--------------|-------------|
| 0x20 | `MILLIS` | `( -- ms)` | Get milliseconds since startup |
| 0x21 | `MICROS` | `( -- us_lo us_hi)` | Get microseconds (64-bit) |
| 0x22 | `DELAY_MS` | `(ms -- )` | Delay milliseconds |
| 0x23 | `DELAY_US` | `(us -- )` | Delay microseconds |

#### MILLIS (0x20)

**Stack**: `( -- ms)`

**Description**: Get milliseconds since system startup.

**Returns**:
- `ms`: Milliseconds (32-bit, wraps after ~49 days)

**Example**:
```forth
\ Get current time
SYS 0x20
\ Stack: ms
```

---

#### MICROS (0x21)

**Stack**: `( -- us_lo us_hi)`

**Description**: Get microseconds since system startup (64-bit).

**Returns**:
- `us_lo`: Lower 32 bits of microseconds
- `us_hi`: Upper 32 bits of microseconds

**Example**:
```forth
\ Get current time in microseconds
SYS 0x21
\ Stack: us_lo us_hi
```

---

#### DELAY_MS (0x22)

**Stack**: `(ms -- )`

**Description**: Blocking delay in milliseconds.

**Parameters**:
- `ms`: Milliseconds to delay

**Example**:
```forth
\ Delay 1000ms (1 second)
1000 SYS 0x22
```

---

#### DELAY_US (0x23)

**Stack**: `(us -- )`

**Description**: Blocking delay in microseconds.

**Parameters**:
- `us`: Microseconds to delay

**Example**:
```forth
\ Delay 100 microseconds
100 SYS 0x23
```

---

### System Operations (0xF0 - 0xFF)

| ID   | Function | Stack Effect | Description |
|------|----------|--------------|-------------|
| 0xFE | `SYSTEM_RESET` | `( -- )` | Reset microcontroller |
| 0xFF | `SYSTEM_INFO` | `( -- addr len)` | Get system info string |

#### SYSTEM_RESET (0xFE)

**Stack**: `( -- )`

**Description**: Perform a system reset (may not return).

**Example**:
```forth
\ Reset the system
SYS 0xFE
```

---

#### SYSTEM_INFO (0xFF)

**Stack**: `( -- addr len)`

**Description**: Get platform identification string.

**Returns**:
- `addr`: Address of string in memory
- `len`: Length of string

**Example**:
```forth
\ Get system info
SYS 0xFF
\ Stack: addr len
\ Can be used with UART to print platform name
```

---

## Usage Examples

### Blink LED Example

```forth
: SETUP
  13 1 SYS 0x00  \ Configure pin 13 as output
  DROP           \ Drop error code
;

: BLINK
  13 1 SYS 0x01  \ Set pin high
  DROP
  500 SYS 0x22   \ Delay 500ms
  13 0 SYS 0x01  \ Set pin low
  DROP
  500 SYS 0x22   \ Delay 500ms
;

: MAIN
  SETUP
  BEGIN BLINK AGAIN
;
```

### UART Echo Example

```forth
: UART-SETUP
  0 115200 SYS 0x10  \ Init UART0 at 115200
  DROP
;

: ECHO-ONCE
  0 SYS 0x12         \ Receive character
  DUP 0 EQ IF        \ Check if no error
    SWAP             \ Get char
    0 SWAP SYS 0x11  \ Send it back
    DROP             \ Drop error
  ELSE
    DROP DROP        \ Drop char and error
  THEN
;

: MAIN
  UART-SETUP
  BEGIN ECHO-ONCE 10 SYS 0x22 AGAIN  \ Loop with 10ms delay
;
```

### Button Input Example

```forth
: SETUP
  2 2 SYS 0x00   \ Configure pin 2 as input with pullup
  DROP
  13 1 SYS 0x00  \ Configure pin 13 as output
  DROP
;

: READ-BUTTON
  2 SYS 0x02     \ Read pin 2
  SWAP DROP      \ Keep value, drop error
;

: UPDATE-LED
  READ-BUTTON
  0 EQ IF        \ If button pressed (active low)
    13 1 SYS 0x01  \ Turn LED on
  ELSE
    13 0 SYS 0x01  \ Turn LED off
  THEN
  DROP           \ Drop error
;

: MAIN
  SETUP
  BEGIN UPDATE-LED 50 SYS 0x22 AGAIN
;
```

---

## Error Handling

All SYS operations that can fail leave an error code on the stack:

- `0`: Success
- Negative: Error (see `errors.def`)

**Best Practice**: Always check error codes in production code.

```forth
: SAFE-GPIO-WRITE
  13 1 SYS 0x01  \ Try to write
  DUP 0 NE IF    \ Check if error
    \ Handle error (e.g., print message)
    DROP
  ELSE
    DROP
  THEN
;
```

---

## Implementation Notes

### For V4-core Developers

The SYS instruction dispatcher in `src/core.cpp`:

```cpp
case v4::Op::SYS:
{
  uint8_t sys_id = code[ip++];

  switch (sys_id) {
    case 0x00:  // GPIO_INIT
      // Pop mode, pin
      // Call v4_hal_gpio_init
      // Push error
      break;
    // ... other cases
  }
  break;
}
```

### Platform Compatibility

SYS IDs are **platform-independent**. HAL implementations must provide consistent behavior across platforms.

---

## Future Extensions

Reserved ID ranges for future use:

- `0x30-0x3F`: I2C operations
- `0x40-0x4F`: SPI operations
- `0x50-0x5F`: PWM operations
- `0x60-0x6F`: ADC operations
- `0x70-0x7F`: DAC operations
- `0x80-0xEF`: User-defined extensions

---

## References

- [HAL API Specification](hal-api.md)
- [V4 Opcode Reference](../include/v4/opcodes.def)
- [Error Codes](../include/v4/errors.def)
