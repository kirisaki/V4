#include "v4/task_platform.h"
#include "v4/internal/vm.h"
#include <cstdint>

/* ========================================================================= */
/* Mock Platform Implementation (for testing)                                */
/* ========================================================================= */

// Mock tick counter (incremented by test code)
static volatile uint32_t mock_tick_ms = 0;
static int critical_nesting = 0;

extern "C" int v4_task_platform_init(Vm *vm, uint32_t time_slice_ms)
{
  (void)vm;
  (void)time_slice_ms;
  // Mock: no actual timer setup needed
  mock_tick_ms = 0;
  return 0;
}

extern "C" int v4_task_platform_deinit(Vm *vm)
{
  (void)vm;
  // Mock: no cleanup needed
  return 0;
}

extern "C" void v4_task_platform_critical_enter(void)
{
  critical_nesting++;
  // Mock: no actual interrupt disable needed in tests
}

extern "C" void v4_task_platform_critical_exit(void)
{
  if (critical_nesting > 0)
  {
    critical_nesting--;
  }
}

extern "C" uint32_t v4_task_platform_get_tick_ms(void)
{
  return mock_tick_ms;
}

// Test helper: advance mock time
extern "C" void mock_task_advance_tick(uint32_t ms)
{
  mock_tick_ms += ms;
}

// Test helper: reset mock time
extern "C" void mock_task_reset_tick(void)
{
  mock_tick_ms = 0;
  critical_nesting = 0;
}
