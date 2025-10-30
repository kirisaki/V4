# V4 VM Bare-Metal Test for RISC-V 32-bit

This directory contains a minimal bare-metal environment for testing V4 VM on RISC-V 32-bit architecture using QEMU.

## Prerequisites

- RISC-V toolchain: `riscv64-unknown-elf-gcc`
- QEMU: `qemu-system-riscv32`

## Build and Run

```bash
make            # Build the bare-metal test
make run        # Run in QEMU
make dump       # Disassemble the binary
make clean      # Clean build artifacts
```

## Current Status

### ✅ Working
- **Bare-metal environment**: Successfully boots on QEMU RISC-V virt machine
- **UART output**: 16550-compatible UART at 0x10000000
- **Basic calculations**: Integer arithmetic without OS
- **Memory management**: Stack and heap properly configured

### Output
```
V4 VM Bare-Metal Test
=====================

Phase 1: Basic UART output
Hello from RISC-V bare-metal!

Phase 2: Simple calculations
10 + 32 = 42
10 * 32 = 320

Test completed successfully!
```

## Architecture

### Memory Layout (QEMU virt machine)
- **RAM**: 0x80000000 - 0x88000000 (128 MB)
- **UART0**: 0x10000000 (16550-compatible)
- **Stack**: Top of RAM (grows downward)

### Files
- `start.S`: Assembly startup code (_start entry point)
- `link.ld`: Linker script for memory layout
- `uart.h`: Simple UART driver (MMIO-based)
- `main_simple.c`: Basic test program
- `main.c`: V4 VM integration (future work)

## Future Work: V4 VM Integration

To integrate V4 VM into the bare-metal environment, the following is needed:

1. **Standard library support**
   - Implement or link malloc/free
   - Provide memcpy, memset, strcmp, etc.
   - Options: newlib, custom minimal implementation

2. **V4 VM source integration**
   - Compile `src/core.cpp` and `src/memory.cpp` with `-fno-exceptions -fno-rtti`
   - Ensure all stdlib dependencies are satisfied

3. **Test bytecode execution**
   - Register simple words (e.g., ADD, MUL)
   - Execute bytecode and verify results via UART

### Note on V4-hal

V4 now supports [V4-hal](https://github.com/kirisaki/V4-hal), a C++17 CRTP HAL implementation. However:
- **bare-metal builds do NOT use V4-hal** (independent Makefile-based build)
- V4-hal is CMake-based and optional (`-DV4_USE_V4HAL=ON`)
- For bare-metal RISC-V, you'll need platform-specific HAL implementation:
  - Direct MMIO access (as in current `uart.h`)
  - Or create V4-hal RISC-V bare-metal port in `V4-hal/ports/riscv32/`

### Example V4 Integration (main.c)

The `main.c` file contains a template for V4 VM integration:
- Creates VM instance with memory configuration
- Registers bytecode words
- Executes simple arithmetic (10 + 32, 7 * 6)
- Outputs results via UART

**Note**: Full V4 integration requires proper stdlib support, which is not yet configured in this bare-metal environment.

## QEMU Command

The default QEMU command:
```bash
qemu-system-riscv32 -M virt -nographic -bios none -kernel bare-metal-test.elf
```

- `-M virt`: RISC-V virt machine (provides UART, RAM)
- `-nographic`: No graphical output, use serial console
- `-bios none`: No BIOS/firmware
- `-kernel`: Load ELF directly at RAM base

## Exit QEMU

Press `Ctrl-A` then `X` to exit QEMU.

## References

- [QEMU RISC-V Documentation](https://www.qemu.org/docs/master/system/target-riscv.html)
- [RISC-V Bare-Metal Examples](https://github.com/riscv-software-src/riscv-tests)
- [16550 UART Specification](https://en.wikipedia.org/wiki/16550_UART)
