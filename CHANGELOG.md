# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
- Added return stack
- Fixed stack helpers to return error correctly
- Added word dictionary and CALL opcode support
- Added new error types (InvalidArg, DictionaryFull, InvalidWordIdx)
- Implemented vm_register_word and vm_get_word API functions
- Implemented vm_exec function
- Added name field to Word structure for debugging and REPL support
- Extended vm_register_word API to accept optional word names

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

[unreleased]: https://github.com/kirisaki/V4/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/kirisaki/V4/releases/tag/v0.1.0