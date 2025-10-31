#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstring>

#include "doctest.h"
#include "mock_hal.h"
#include "v4/errors.hpp"
#include "v4/hal.h"
#include "v4/internal/vm.h"
#include "v4/opcodes.hpp"
#include "v4/sys_ids.h"
#include "v4/vm_api.h"

/* Mock HAL console control functions */
extern "C" void mock_hal_console_inject_input(const char* data, int len);
extern "C" const char* mock_hal_console_get_output(int* out_len);

/* Helper to emit bytecode */
static void emit8(v4_u8* code, int* k, v4_u8 byte)
{
  code[(*k)++] = byte;
}

static void emit32(v4_u8* code, int* k, v4_u32 val)
{
  code[(*k)++] = (val >> 0) & 0xFF;
  code[(*k)++] = (val >> 8) & 0xFF;
  code[(*k)++] = (val >> 16) & 0xFF;
  code[(*k)++] = (val >> 24) & 0xFF;
}

extern "C" v4_err vm_exec_raw(Vm* vm, const v4_u8* code, int code_len);

TEST_CASE("SYS GPIO_INIT")
{
  mock_hal_reset();

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[32];
  int k = 0;

  // Push pin 13, mode 1 (OUTPUT)
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 13);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 1);

  // SYS GPIO_INIT
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_GPIO_INIT);

  // RET
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  // Execute
  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Check stack: should have error code (0)
  CHECK(vm_ds_depth_public(&vm) == 1);
  CHECK(vm_ds_peek_public(&vm, 0) == 0);

  // Check mock HAL state
  CHECK(mock_hal_gpio_get_mode(13) == V4_HAL_GPIO_MODE_OUTPUT);
}

TEST_CASE("SYS GPIO_WRITE and GPIO_READ")
{
  mock_hal_reset();

  Vm vm{};
  vm_reset(&vm);

  // First init pin 10 as output
  hal_gpio_mode(10, HAL_GPIO_OUTPUT);

  v4_u8 code[64];
  int k = 0;

  // Write high: pin=10, value=1
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 10);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 1);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_GPIO_WRITE);

  // Drop error code
  emit8(code, &k, static_cast<v4_u8>(v4::Op::DROP));

  // Read back: pin=10
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 10);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_GPIO_READ);

  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Stack: [value, err]
  CHECK(vm_ds_depth_public(&vm) == 2);
  CHECK(vm_ds_peek_public(&vm, 0) == 0);  // err = 0
  CHECK(vm_ds_peek_public(&vm, 1) == 1);  // value = 1
}

TEST_CASE("SYS UART_INIT and UART_PUTC")
{
  mock_hal_reset();

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[64];
  int k = 0;

  // Init UART0 at 115200
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 0);  // port
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 115200);  // baudrate
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_UART_INIT);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::DROP));  // Drop error

  // Send 'A' (65)
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 0);  // port
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 65);  // char 'A'
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_UART_PUTC);

  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Check transmitted data
  int tx_len;
  const char* tx_buf = mock_hal_uart_get_tx(0, &tx_len);
  REQUIRE(tx_buf != nullptr);
  CHECK(tx_len == 1);
  CHECK(tx_buf[0] == 'A');
}

TEST_CASE("SYS UART_GETC")
{
  mock_hal_reset();

  // Init UART and inject data
  hal_uart_config_t config = {115200, 8, 1, 0};
  hal_uart_open(0, &config);
  mock_hal_uart_inject_rx(0, "X", 1);

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[32];
  int k = 0;

  // Receive from UART0
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 0);  // port
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_UART_GETC);

  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Stack: [char, err]
  CHECK(vm_ds_depth_public(&vm) == 2);
  CHECK(vm_ds_peek_public(&vm, 0) == 0);    // err = 0
  CHECK(vm_ds_peek_public(&vm, 1) == 'X');  // char = 'X'
}

TEST_CASE("SYS MILLIS")
{
  mock_hal_reset();
  mock_hal_set_millis(12345);

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;

  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_MILLIS);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  CHECK(vm_ds_depth_public(&vm) == 1);
  CHECK(vm_ds_peek_public(&vm, 0) == 12345);
}

TEST_CASE("SYS MICROS")
{
  mock_hal_reset();
  mock_hal_set_micros(0x123456789ABCULL);

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;

  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_MICROS);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Stack: [us_lo, us_hi]
  CHECK(vm_ds_depth_public(&vm) == 2);
  uint32_t us_hi = static_cast<uint32_t>(vm_ds_peek_public(&vm, 0));
  uint32_t us_lo = static_cast<uint32_t>(vm_ds_peek_public(&vm, 1));

  uint64_t reconstructed = (static_cast<uint64_t>(us_hi) << 32) | us_lo;
  CHECK(reconstructed == 0x123456789ABCULL);
}

TEST_CASE("SYS DELAY_MS")
{
  mock_hal_reset();
  mock_hal_set_millis(1000);

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[32];
  int k = 0;

  // Delay 500ms
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 500);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_DELAY_MS);

  // Get millis after delay
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_MILLIS);

  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Should be 1000 + 500 = 1500
  CHECK(vm_ds_depth_public(&vm) == 1);
  CHECK(vm_ds_peek_public(&vm, 0) == 1500);
}

TEST_CASE("SYS DELAY_US")
{
  mock_hal_reset();
  mock_hal_set_micros(10000);

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[32];
  int k = 0;

  // Delay 250us
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 250);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_DELAY_US);

  // Get micros after delay (lower 32 bits)
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_MICROS);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::DROP));  // Drop us_hi

  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Should be 10000 + 250 = 10250
  CHECK(vm_ds_depth_public(&vm) == 1);
  CHECK(vm_ds_peek_public(&vm, 0) == 10250);
}

TEST_CASE("SYS SYSTEM_INFO")
{
  mock_hal_reset();

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;

  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_SYSTEM_INFO);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // SYSTEM_INFO returns HAL_ERR_NOTSUP (-6) in new HAL API
  // Stack: [0, 0, err]
  CHECK(vm_ds_depth_public(&vm) == 3);
  v4_i32 hal_err = vm_ds_peek_public(&vm, 0);
  CHECK(hal_err == HAL_ERR_NOTSUP);  // -6
}

TEST_CASE("SYS error handling - invalid pin")
{
  mock_hal_reset();

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[32];
  int k = 0;

  // Try to init invalid pin (e.g., 999)
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 999);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 1);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_GPIO_INIT);

  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Should have error code on stack
  CHECK(vm_ds_depth_public(&vm) == 1);
  v4_i32 hal_err = vm_ds_peek_public(&vm, 0);
  CHECK(hal_err == HAL_ERR_PARAM);  // -1 (invalid parameter)
}

TEST_CASE("SYS EMIT")
{
  mock_hal_reset();

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[32];
  int k = 0;

  // Emit 'A' (65)
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 65);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_EMIT);

  // Drop error code
  emit8(code, &k, static_cast<v4_u8>(v4::Op::DROP));

  // Emit 'B' (66)
  emit8(code, &k, static_cast<v4_u8>(v4::Op::LIT));
  emit32(code, &k, 66);
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_EMIT);

  // Drop error code
  emit8(code, &k, static_cast<v4_u8>(v4::Op::DROP));

  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Check console output
  int out_len = 0;
  const char* output = mock_hal_console_get_output(&out_len);
  REQUIRE(out_len == 2);
  CHECK(output[0] == 'A');
  CHECK(output[1] == 'B');
}

TEST_CASE("SYS KEY")
{
  mock_hal_reset();

  // Inject input characters
  mock_hal_console_inject_input("XY", 2);

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[32];
  int k = 0;

  // Read first character
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_KEY);

  // Drop error code
  emit8(code, &k, static_cast<v4_u8>(v4::Op::DROP));

  // Read second character
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_KEY);

  // Drop error code
  emit8(code, &k, static_cast<v4_u8>(v4::Op::DROP));

  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Stack: [first_char, second_char]
  CHECK(vm_ds_depth_public(&vm) == 2);
  CHECK(vm_ds_peek_public(&vm, 1) == 'X');
  CHECK(vm_ds_peek_public(&vm, 0) == 'Y');
}

TEST_CASE("SYS KEY - no data available")
{
  mock_hal_reset();

  // No input injected

  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;

  // Try to read character
  emit8(code, &k, static_cast<v4_u8>(v4::Op::SYS));
  emit8(code, &k, V4_SYS_KEY);

  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  v4_err err = vm_exec_raw(&vm, code, k);
  REQUIRE(err == 0);

  // Stack: [char, err]
  // In new HAL API, hal_console_read returns 0 when no data available (not an error)
  CHECK(vm_ds_depth_public(&vm) == 2);
  v4_i32 hal_err = vm_ds_peek_public(&vm, 0);
  CHECK(hal_err == HAL_OK);  // 0 (success, but 0 bytes read)
}
