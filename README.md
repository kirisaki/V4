# V4 Engine

A lightweight Forth-compatible bytecode VM written in C++17.

## Features

- Stack-based architecture with 32-bit cells
- **Preemptive multitasking system** (v0.9.0+)
  - Priority-based scheduler with up to 8 concurrent tasks
  - Inter-task message passing with 16-message queue
  - Task sleep, yield, and critical sections
  - Platform-abstracted timer interrupts
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

## Task System

V4 includes a preemptive multitasking system with **pluggable task backends**:

### Task Backends

**Custom Backend** (default):
- V4's own priority-based + round-robin scheduler
- Suitable for bare-metal and testing
- Minimal dependencies

**FreeRTOS Backend**:
- Native FreeRTOS task integration
- For ESP32, ESP-IDF, and other FreeRTOS platforms
- Full FreeRTOS scheduler features

Configure backend via CMake:
```bash
# Custom backend (default)
cmake -B build -DV4_TASK_BACKEND=CUSTOM

# FreeRTOS backend
cmake -B build -DV4_TASK_BACKEND=FREERTOS -DFREERTOS_PATH=/path/to/freertos
```

For ESP-IDF, FreeRTOS is provided automatically (no `FREERTOS_PATH` needed).

### Memory Footprint
- Task scheduler: ~400 bytes
- Per-task overhead: 32 bytes + stack allocation
- Total VM with tasks: ~42KB (Release build with LTO)

### API Example
```c
#include "v4/task.h"

// Initialize task system (10ms time slice)
vm_task_init(vm, 10);

// Spawn a task
int task_id = vm_task_spawn(vm, word_idx, priority, ds_size, rs_size);

// Task control
vm_task_sleep(vm, 100);  // Sleep 100ms
vm_task_yield(vm);       // Yield to other tasks

// Inter-task messaging
vm_task_send(vm, target_task, msg_type, data);
vm_task_receive(vm, msg_type, &data, &src_task);
```

### VM Opcodes (0x90-0x9A)
- `TASK_SPAWN`, `TASK_EXIT`, `TASK_SLEEP`, `TASK_YIELD`
- `CRITICAL_ENTER`, `CRITICAL_EXIT`
- `TASK_SEND`, `TASK_RECEIVE`, `TASK_RECEIVE_BLOCKING`
- `TASK_SELF`, `TASK_COUNT`

See [v4/task.h](include/v4/task.h) for full API documentation.

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