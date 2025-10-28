# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[unreleased]: https://github.com/kirisaki/V4/compare/v0.2.1...HEAD
[0.2.1]: https://github.com/kirisaki/V4/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/kirisaki/V4/compare/v0.1.3...v0.2.0
[0.1.3]: https://github.com/kirisaki/V4/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/kirisaki/V4/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/kirisaki/V4/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/kirisaki/V4/releases/tag/v0.1.0