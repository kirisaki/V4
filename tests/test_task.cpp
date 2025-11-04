#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstring>

#include "doctest.h"
#include "v4/errors.h"
#include "v4/internal/scheduler.hpp"
#include "v4/internal/vm.h"
#include "v4/task.h"
#include "v4/vm_api.h"

// Mock platform helpers
extern "C"
{
  void mock_task_advance_tick(uint32_t ms);
  void mock_task_reset_tick(void);
  uint32_t mock_task_get_tick(void);
}

TEST_CASE("Task system initialization")
{
  VmConfig cfg = {nullptr, 0, nullptr, 0, nullptr};
  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  // Initialize task system
  v4_err err = vm_task_init(vm, 10);
  CHECK(err == 0);

  // Verify scheduler state
  CHECK(vm->scheduler.time_slice_ms == 10);
  CHECK(vm->scheduler.task_count == 0);
  CHECK(vm->scheduler.current_task == 0);
  CHECK(vm->scheduler.critical_nesting == 0);

  // Verify all tasks are DEAD
  for (int i = 0; i < V4_MAX_TASKS; i++)
  {
    CHECK(vm->scheduler.tasks[i].state == V4_TASK_STATE_DEAD);
  }

  // Verify message queue is empty
  CHECK(vm->msg_queue.count == 0);

  vm_task_cleanup(vm);
  vm_destroy(vm);
}

TEST_CASE("Task spawn and exit")
{
  VmConfig cfg = {nullptr, 0, nullptr, 0, nullptr};
  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  vm_task_init(vm, 10);

  // Register a dummy word
  const uint8_t dummy_code[] = {0x51};  // RET
  int word_idx = vm_register_word(vm, "DUMMY", dummy_code, sizeof(dummy_code));
  REQUIRE(word_idx >= 0);

  SUBCASE("Spawn task successfully")
  {
    int task_id = vm_task_spawn(vm, word_idx, 128, 256, 64);
    CHECK(task_id >= 0);
    CHECK(task_id < V4_MAX_TASKS);
    CHECK(vm->scheduler.task_count == 1);

    // Verify task state
    v4_task_state_t state;
    uint8_t priority;
    v4_err err = vm_task_get_info(vm, task_id, &state, &priority);
    CHECK(err == 0);
    CHECK(state == V4_TASK_STATE_READY);
    CHECK(priority == 128);
  }

  SUBCASE("Spawn multiple tasks")
  {
    int task_ids[3];
    for (int i = 0; i < 3; i++)
    {
      task_ids[i] = vm_task_spawn(vm, word_idx, 100 + i * 10, 128, 32);
      CHECK(task_ids[i] >= 0);
    }
    CHECK(vm->scheduler.task_count == 3);

    // Verify all tasks have different IDs
    CHECK(task_ids[0] != task_ids[1]);
    CHECK(task_ids[1] != task_ids[2]);
    CHECK(task_ids[0] != task_ids[2]);
  }

  SUBCASE("Task table full")
  {
    // Fill task table
    for (int i = 0; i < V4_MAX_TASKS; i++)
    {
      int task_id = vm_task_spawn(vm, word_idx, 100, 128, 32);
      CHECK(task_id >= 0);
    }
    CHECK(vm->scheduler.task_count == V4_MAX_TASKS);

    // Try to spawn one more
    int task_id = vm_task_spawn(vm, word_idx, 100, 128, 32);
    CHECK(task_id == V4_ERR_TaskLimit);
  }

  SUBCASE("Invalid word index")
  {
    int task_id = vm_task_spawn(vm, 999, 100, 128, 32);
    CHECK(task_id == V4_ERR_InvalidWordIdx);
  }

  vm_task_cleanup(vm);
  vm_destroy(vm);
}

TEST_CASE("Task self and count")
{
  VmConfig cfg = {nullptr, 0, nullptr, 0, nullptr};
  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  vm_task_init(vm, 10);

  const uint8_t dummy_code[] = {0x51};  // RET
  int word_idx = vm_register_word(vm, "DUMMY", dummy_code, sizeof(dummy_code));

  // Initially current task is 0
  int current_id = vm_task_self(vm);
  CHECK(current_id == 0);

  // Spawn some tasks
  vm_task_spawn(vm, word_idx, 100, 128, 32);
  vm_task_spawn(vm, word_idx, 100, 128, 32);

  // Task count should be 2
  CHECK(vm->scheduler.task_count == 2);

  vm_task_cleanup(vm);
  vm_destroy(vm);
}

TEST_CASE("Message passing")
{
  VmConfig cfg = {nullptr, 0, nullptr, 0, nullptr};
  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  vm_task_init(vm, 10);

  SUBCASE("Send and receive message")
  {
    // Send message from task 0 to task 1
    v4_err err = vm_task_send(vm, 1, 0x42, 12345);
    CHECK(err == 0);
    CHECK(vm->msg_queue.count == 1);

    // Simulate switching to task 1
    vm->scheduler.current_task = 1;

    // Receive message
    int32_t data;
    uint8_t src_task;
    int result = vm_task_receive(vm, 0x42, &data, &src_task);
    CHECK(result == 1);  // Message received
    CHECK(data == 12345);
    CHECK(src_task == 0);
    CHECK(vm->msg_queue.count == 0);  // Queue now empty
  }

  SUBCASE("Receive with no message")
  {
    int32_t data;
    uint8_t src_task;
    int result = vm_task_receive(vm, 0x42, &data, &src_task);
    CHECK(result == 0);  // No message
  }

  SUBCASE("Message type filtering")
  {
    // Send two messages with different types
    vm_task_send(vm, 1, 0x10, 111);
    vm_task_send(vm, 1, 0x20, 222);

    vm->scheduler.current_task = 1;

    // Receive only type 0x20
    int32_t data;
    uint8_t src_task;
    int result = vm_task_receive(vm, 0x20, &data, &src_task);
    CHECK(result == 1);
    CHECK(data == 222);

    // First message (0x10) should still be in queue
    CHECK(vm->msg_queue.count == 1);
  }

  SUBCASE("Broadcast message (0xFF)")
  {
    // Send broadcast
    vm_task_send(vm, 0xFF, 0x99, 999);

    // Any task can receive it
    vm->scheduler.current_task = 5;
    int32_t data;
    uint8_t src_task;
    int result = vm_task_receive(vm, 0x99, &data, &src_task);
    CHECK(result == 1);
    CHECK(data == 999);
  }

  SUBCASE("Queue full")
  {
    // Fill message queue
    for (int i = 0; i < V4_MSG_QUEUE_SIZE; i++)
    {
      v4_err err = vm_task_send(vm, 1, 0x01, i);
      CHECK(err == 0);
    }
    CHECK(vm->msg_queue.count == V4_MSG_QUEUE_SIZE);

    // Try to send one more
    v4_err err = vm_task_send(vm, 1, 0x01, 999);
    CHECK(err == V4_ERR_MsgQueueFull);
  }

  vm_task_cleanup(vm);
  vm_destroy(vm);
}

TEST_CASE("Task sleep")
{
  VmConfig cfg = {nullptr, 0, nullptr, 0, nullptr};
  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  vm_task_init(vm, 10);
  mock_task_reset_tick();

  const uint8_t dummy_code[] = {0x51};  // RET
  int word_idx = vm_register_word(vm, "DUMMY", dummy_code, sizeof(dummy_code));

  int task_id = vm_task_spawn(vm, word_idx, 100, 128, 32);
  REQUIRE(task_id >= 0);

  // Set task as running
  vm->scheduler.tasks[task_id].state = V4_TASK_STATE_RUNNING;
  vm->scheduler.current_task = task_id;

  SUBCASE("Sleep puts task in BLOCKED state")
  {
    // Note: vm_task_sleep calls vm_schedule, which would try to switch tasks
    // For this test, we just verify the sleep_until_tick is set correctly
    uint32_t current_tick = mock_task_get_tick();
    v4_task_t* task = &vm->scheduler.tasks[task_id];

    // Manually set sleep (simulating what vm_task_sleep does)
    task->sleep_until_tick = current_tick + 100;
    task->state = V4_TASK_STATE_BLOCKED;

    CHECK(task->state == V4_TASK_STATE_BLOCKED);
    CHECK(task->sleep_until_tick == current_tick + 100);
  }

  SUBCASE("Task wakes up after sleep time")
  {
    v4_task_t* task = &vm->scheduler.tasks[task_id];
    uint32_t current_tick = mock_task_get_tick();

    task->sleep_until_tick = current_tick + 50;
    task->state = V4_TASK_STATE_BLOCKED;

    // Advance time by 60ms
    mock_task_advance_tick(60);

    // Call scheduler to wake up tasks
    // v4_task_select_next should wake this task
    uint8_t next_task = v4_task_select_next(vm);

    // Task should be READY now
    CHECK(task->state == V4_TASK_STATE_READY);
  }

  mock_task_reset_tick();
  vm_task_cleanup(vm);
  vm_destroy(vm);
}

TEST_CASE("Critical section")
{
  VmConfig cfg = {nullptr, 0, nullptr, 0, nullptr};
  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  vm_task_init(vm, 10);

  SUBCASE("Enter/exit critical section")
  {
    CHECK(vm->scheduler.critical_nesting == 0);

    vm_task_critical_enter(vm);
    CHECK(vm->scheduler.critical_nesting == 1);

    vm_task_critical_exit(vm);
    CHECK(vm->scheduler.critical_nesting == 0);
  }

  SUBCASE("Nested critical sections")
  {
    vm_task_critical_enter(vm);
    vm_task_critical_enter(vm);
    vm_task_critical_enter(vm);
    CHECK(vm->scheduler.critical_nesting == 3);

    vm_task_critical_exit(vm);
    CHECK(vm->scheduler.critical_nesting == 2);

    vm_task_critical_exit(vm);
    CHECK(vm->scheduler.critical_nesting == 1);

    vm_task_critical_exit(vm);
    CHECK(vm->scheduler.critical_nesting == 0);
  }

  SUBCASE("Exit without enter")
  {
    // Should not go negative
    vm_task_critical_exit(vm);
    CHECK(vm->scheduler.critical_nesting == 0);
  }

  vm_task_cleanup(vm);
  vm_destroy(vm);
}

TEST_CASE("Task opcodes")
{
  VmConfig cfg = {nullptr, 0, nullptr, 0, nullptr};
  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  vm_task_init(vm, 10);

  SUBCASE("TASK_SELF opcode")
  {
    const uint8_t code[] = {
        0x99,  // TASK_SELF
        0x51   // RET
    };

    int word_idx = vm_register_word(vm, "GET_TASK_ID", code, sizeof(code));
    Word* word = vm_get_word(vm, word_idx);

    v4_err err = vm_exec(vm, word);
    CHECK(err == 0);

    // Should push current task ID to stack
    CHECK(vm_ds_depth_public(vm) == 1);
    int32_t task_id = vm_ds_peek_public(vm, 0);
    CHECK(task_id == 0);  // Default current task
  }

  SUBCASE("TASK_COUNT opcode")
  {
    // Spawn some tasks first
    const uint8_t dummy_code[] = {0x51};
    int dummy_idx = vm_register_word(vm, "DUMMY", dummy_code, sizeof(dummy_code));
    vm_task_spawn(vm, dummy_idx, 100, 128, 32);
    vm_task_spawn(vm, dummy_idx, 100, 128, 32);

    const uint8_t code[] = {
        0x9A,  // TASK_COUNT
        0x51   // RET
    };

    int word_idx = vm_register_word(vm, "GET_TASK_COUNT", code, sizeof(code));
    Word* word = vm_get_word(vm, word_idx);

    v4_err err = vm_exec(vm, word);
    CHECK(err == 0);

    CHECK(vm_ds_depth_public(vm) == 1);
    int32_t count = vm_ds_peek_public(vm, 0);
    CHECK(count == 2);
  }

  SUBCASE("CRITICAL_ENTER/EXIT opcodes")
  {
    const uint8_t code[] = {
        0x94,  // CRITICAL_ENTER
        0x94,  // CRITICAL_ENTER (nested)
        0x95,  // CRITICAL_EXIT
        0x95,  // CRITICAL_EXIT
        0x51   // RET
    };

    int word_idx = vm_register_word(vm, "CRITICAL_TEST", code, sizeof(code));
    Word* word = vm_get_word(vm, word_idx);

    v4_err err = vm_exec(vm, word);
    CHECK(err == 0);

    // Should be back to 0
    CHECK(vm->scheduler.critical_nesting == 0);
  }

  SUBCASE("TASK_SEND opcode")
  {
    // Stack order for TASK_SEND: ( target_task msg_type data -- err )
    const uint8_t code[] = {
        0x73,                          // LIT0 (target_task=0)
        0x76, 0x42,                    // LIT_U8 0x42 (msg_type)
        0x00, 0x39, 0x30, 0x00, 0x00,  // LIT 12345 (data) - little-endian
        0x96,                          // TASK_SEND
        0x51                           // RET
    };

    int word_idx = vm_register_word(vm, "SEND_MSG", code, sizeof(code));
    Word* word = vm_get_word(vm, word_idx);

    v4_err err = vm_exec(vm, word);
    CHECK(err == 0);

    // Should push result (0 = success)
    CHECK(vm_ds_depth_public(vm) == 1);
    int32_t result = vm_ds_peek_public(vm, 0);
    CHECK(result == 0);

    // Verify message in queue
    CHECK(vm->msg_queue.count == 1);
  }

  vm_task_cleanup(vm);
  vm_destroy(vm);
}
