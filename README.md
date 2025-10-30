# V4 Virtual Machine

A lightweight Forth-compatible bytecode VM written in C++17.

## Features

- Stack-based architecture with 32-bit cells
- Memory-mapped I/O support
- Little-endian byte ordering
- No exceptions, no RTTI
- Zero dependencies (except tests)

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Building with V4-hal (C++17 CRTP HAL)

V4 can optionally use the [V4-hal](https://github.com/kirisaki/V4-hal) C++17 CRTP implementation for zero-cost hardware abstraction:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DV4_USE_V4HAL=ON
cmake --build build -j
```

This provides:
- Zero-cost abstraction via compile-time polymorphism
- Platform support: POSIX, ESP32, CH32V203
- Minimal runtime footprint (~5.7KB for GPIO+Timer)
- Backward compatible with existing `v4_hal_*` API

## Testing

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DV4_BUILD_TESTS=ON
cmake --build build -j
cd build && ctest --output-on-failure
```

### Testing with V4-hal

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DV4_BUILD_TESTS=ON -DV4_USE_V4HAL=ON
cmake --build build -j
cd build && ctest --output-on-failure
```

## License

Licensed under either of:

- MIT License ([LICENSE-MIT](LICENSE-MIT))
- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE))

at your option.