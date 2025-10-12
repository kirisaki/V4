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

## Testing

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DV4_BUILD_TESTS=ON
cmake --build build -j
cd build && ctest --output-on-failure
```

## License

Licensed under either of:

- MIT License ([LICENSE-MIT](LICENSE-MIT))
- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE))

at your option.