# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.8.0] - 2025-11-02

### Added
- REPL/debugger support API: `vm_find_word()` for searching words by name
  - Case-sensitive linear search through word dictionary
  - Returns word index (>=0) on success, negative value on failure
  - Enables external tools (V4-front, V4-cli) to lookup words without maintaining separate mappings
  - Comprehensive test coverage in `test_api_extensions.cpp`
- REPL/debugger support API: `vm_ds_copy_to_array()` for bulk data stack retrieval
  - Efficient one-shot copy of entire stack to array (bottom to top)
  - Supports partial copy when max_count < stack depth
  - NULL-safe parameter validation
  - Enables efficient stack display and V4-link protocol stack transfers
- REPL/debugger support API: Return stack inspection APIs
  - `vm_rs_depth_public()`: Get return stack depth
  - `vm_rs_copy_to_array()`: Bulk copy return stack to array
  - Same behavior as data stack APIs but for return stack
  - Enables debugger to inspect call frames and local variables

### Changed
- Binary size increase: +1KB (21KB → 22KB) for new REPL/debugger APIs

## [0.7.0] - 2025-11-01

### Changed
- **BREAKING**: Migrated HAL API from `v4/v4_hal.h` to `v4/hal.h`
  - Complete migration to V4-hal library API
  - Replaced `v4_hal_*` functions with `hal_*` equivalents
  - Updated all SYS instruction implementations in VM core
  - GPIO: `v4_hal_gpio_*` → `hal_gpio_*` with proper type casting
  - UART: Character-based API → Buffer-based API with handle management
  - Timer: `v4_hal_millis/micros/delay_*` → `hal_*`
  - Console: `v4_hal_putc/getc` → `hal_console_write/read`

### Added
- UART handle management (static array for up to 4 ports)
- Proper type casting for GPIO values (`hal_gpio_value_t`)
- Comprehensive UART configuration with default 8N1 settings

### Removed
- Dependency on old `v4_hal.h` API

### Note
- System operations (`v4_hal_system_reset`, `v4_hal_system_info`) temporarily return `HAL_ERR_NOTSUP`
- These will be implemented in V4-hal library in a future release

### Tested
- ESP32-C6 M5NanoC6 with v4-blink and v4-repl-demo examples

## [0.6.0] - 2025-10-31

### Added
- V4-hal C++17 CRTP HAL implementation integration
  - Optional V4-hal library support via `V4_USE_V4HAL` option
  - Local path reference (`V4HAL_LOCAL_PATH`) or FetchContent from GitHub
  - Bridge layer (`src/hal_wrapper.cpp`) mapping `v4_hal_*` to V4-hal's `hal_*` API
  - Zero-cost abstraction benefits from C++17 CRTP architecture
  - Backward compatible with existing `v4_hal_*` API
  - All existing tests pass with V4-hal enabled

## [0.4.1] - 2025-10-29

### Changed
- Refactored error handling to use V4_ERR() macro for improved code readability
  - Replaced 68 instances of `static_cast<v4_err>(Err::XXX)` with shorter `V4_ERR(XXX)` macro
  - No runtime cost - compiler generates identical machine code
  - Reduced source code verbosity by ~1200 characters

## [0.4.0] - 2025-10-29

### Added
- Extended arithmetic operations:
  - DIVU (0x15), MODU (0x16): Unsigned division and modulo
  - INC (0x17), DEC (0x18): Increment and decrement TOS
- Extended comparison operations:
  - LTU (0x26), LEU (0x27): Unsigned less-than comparisons
- Bitwise shift operations:
  - SHL (0x2C), SHR (0x2D), SAR (0x2E): Left shift, logical right shift, arithmetic right shift
- Extended memory access operations:
  - LOAD8U (0x32), LOAD16U (0x33): Unsigned 8/16-bit loads
  - STORE8 (0x34), STORE16 (0x35): 8/16-bit stores
  - LOAD8S (0x36), LOAD16S (0x37): Signed 8/16-bit loads with sign extension
- Control flow and compact literal operations:
  - SELECT (0x43): Ternary operator (flag b a SELECT → flag ? a : b)
  - LIT0 (0x73), LIT1 (0x74), LITN1 (0x75): Common constant literals
  - LIT_U8 (0x76), LIT_I8 (0x77), LIT_I16 (0x78): Compact immediate values
- Local variable support:
  - Frame pointer (fp) added to VM structure
  - LGET (0x79), LSET (0x7A), LTEE (0x7B): General local variable access
  - LGET0 (0x7C), LGET1 (0x7D), LSET0 (0x7E), LSET1 (0x7F): Optimized local access
  - LINC (0x80), LDEC (0x81): Local variable increment/decrement
- Comprehensive test coverage for all 31 new opcodes

### Changed
- Local variables stored in return stack with frame pointer for efficient access

### Added (Build System)
- V4_ENABLE_LTO option to enable Link Time Optimization (default: OFF)
  - 35% size reduction in core VM code (23KB → 15KB stripped) when enabled
  - IPO (Interprocedural Optimization) support detection via CheckIPOSupported
  - Note: LTO is not compatible with sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer)

## [0.3.0] - 2025-10-28

### Added
- REPL support APIs for interactive programming:
  - vm_reset_dictionary: Reset word dictionary while preserving stacks
  - vm_reset_stacks: Reset stacks while preserving dictionary
  - vm_ds_push, vm_ds_pop, vm_ds_clear: Direct stack manipulation APIs
  - vm_ds_snapshot, vm_ds_restore, vm_ds_snapshot_free: Stack snapshot/restore for preserving state across VM operations
- Comprehensive test coverage for all new REPL APIs
- Arena memory allocator for embedded systems:
  - Linear bump allocator without individual free
  - Alignment support for various data types
  - v4_arena_init, v4_arena_alloc, v4_arena_reset APIs
  - Eliminates need for malloc/free in constrained environments
- Hardware Abstraction Layer (HAL) for embedded platforms:
  - GPIO API: pin initialization, read/write
  - UART API: serial communication (init, putc, getc)
  - Timer API: milliseconds/microseconds, delays
  - System API: reset and platform info
- SYS instruction (0x60) for HAL access from bytecode:
  - 10 system call IDs for GPIO, UART, and Timer operations
  - Stack-based parameter passing
  - Error code returns for all operations
- Mock HAL implementation for unit testing
- Comprehensive documentation (hal-api.md, sys-opcodes.md)

### Changed
- vm_reset now implemented as vm_reset_dictionary + vm_reset_stacks for better modularity
- All test targets now link with mock_hal for HAL function resolution
- VmConfig now includes optional arena field for memory management
- vm_register_word uses arena allocator when available (falls back to malloc)
- vm_destroy and vm_reset_dictionary skip free() when using arena
- Word name memory is now managed by arena or malloc depending on configuration
- CMakeLists.txt reorganized with clear sections and better structure
- Build system now uses helper function for test targets to reduce repetition
- Added project version to CMake configuration (0.2.1)
- Improved include directory handling with generator expressions

### Removed
- v4repl tool (moved to separate repository)

### Fixed
- Arena alignment test failure on macOS by ensuring test buffer is 16-byte aligned
- Memory leak in test_vm when using vm_register_word without cleanup (detected by AddressSanitizer)

### Added (Build System)
- V4_BUILD_TOOLS option to control tool building (default: ON)
- V4_ENABLE_MOCK_HAL option to control mock HAL inclusion (default: ON)
- V4_OPTIMIZE_SIZE option to control size optimization (default: ON)
- CMake configuration summary displayed during configuration
- Installation targets for library and headers
- V4::mock_hal alias target for easier linking

### Added (CI/CD)
- Build options matrix CI job to test all CMake option combinations
- Tests for minimal builds (library-only configurations)
- Tests for size-optimized vs non-optimized builds
- Verification of all new CMake options (V4_BUILD_TOOLS, V4_ENABLE_MOCK_HAL, V4_OPTIMIZE_SIZE)

## [0.2.1] - 2025-10-27

### Fixed
- Memory leaks in test cases where word names were not properly freed
- Critical: vm_destroy now properly frees word names allocated by vm_register_word

## [0.2.0] - 2025-10-27

### Added
- Return stack operations (TOR, FROMR, RFETCH)
- Word dictionary and CALL opcode for function calls
- New error types: InvalidArg, DictionaryFull, InvalidWordIdx
- vm_register_word and vm_get_word API functions
- vm_exec function for executing word entries
- Word name field for debugging and REPL support
- Stack inspection APIs: vm_ds_depth_public and vm_ds_peek_public

### Changed
- **BREAKING**: vm_register_word now accepts optional name parameter
- **BREAKING**: Word structure now includes name field
- Fixed stack helpers to return errors correctly

## [0.1.3] - 2025-10-26
- Fix bit length of literal number

## [0.1.2] - 2025-10-25
- Describe PrimKind in opcodes.def
- Buld in Windows

## [0.1.1] - 2025-10-25
- Add lables to opcodes

## [0.1.0] - 2025-10-12

### Added
- Initial release of V4 VM
- Stack-based bytecode interpreter with 32-bit cells
- Basic stack operations (LIT, DUP, DROP, SWAP, OVER)
- Arithmetic operations (ADD, SUB, MUL, DIV, MOD)
- Comparison operations (EQ, NE, LT, LE, GT, GE)
- Bitwise operations (AND, OR, XOR, INVERT)
- Control flow instructions (JMP, JZ, JNZ, RET)
- Memory operations (LOAD, STORE) with alignment checking
- Memory-mapped I/O (MMIO) support
- C and C++ public API
- Comprehensive test suite with doctest

[unreleased]: https://github.com/V4-project/V4/compare/v0.8.0...HEAD
[0.8.0]: https://github.com/V4-project/V4/compare/v0.7.0...v0.8.0
[0.7.0]: https://github.com/V4-project/V4/compare/v0.6.0...v0.7.0
[0.6.0]: https://github.com/V4-project/V4/compare/v0.4.1...v0.6.0
[0.4.1]: https://github.com/kirisaki/V4/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/kirisaki/V4/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/kirisaki/V4/compare/v0.2.1...v0.3.0
[0.2.1]: https://github.com/kirisaki/V4/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/kirisaki/V4/compare/v0.1.3...v0.2.0
[0.1.3]: https://github.com/kirisaki/V4/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/kirisaki/V4/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/kirisaki/V4/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/kirisaki/V4/releases/tag/v0.1.0