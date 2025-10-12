# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[unreleased]: https://github.com/YOUR_USERNAME/V4/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/YOUR_USERNAME/V4/releases/tag/v0.1.0