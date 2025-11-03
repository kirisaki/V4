#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /* ========================================================================= */
  /* Task System Internal Data Structures                                     */
  /* ========================================================================= */

  /**
   * @brief Maximum number of tasks
   */
#define V4_MAX_TASKS 8

  /**
   * @brief Message queue size
   */
#define V4_MSG_QUEUE_SIZE 16

  /**
   * @brief Task state enumeration
   */
  typedef enum
  {
    V4_TASK_STATE_DEAD = 0,    /**< Unused or terminated */
    V4_TASK_STATE_READY,       /**< Ready to run */
    V4_TASK_STATE_RUNNING,     /**< Currently running */
    V4_TASK_STATE_BLOCKED,     /**< Blocked (sleep/receive) */
  } v4_task_state_t;

  /**
   * @brief Task Control Block (TCB)
   *
   * Size: 32 bytes (with alignment)
   * Each task maintains its own execution context.
   */
  typedef struct
  {
    /* === Execution Context (16 bytes) === */
    uint16_t word_idx;  /**< Word index to execute */
    uint16_t pc;        /**< Program counter (bytecode offset) */

    int32_t *ds_base;  /**< Data stack base address */
    int32_t *rs_base;  /**< Return stack base address */
    uint8_t ds_depth;  /**< Data stack depth */
    uint8_t rs_depth;  /**< Return stack depth */

    /* === Task State (8 bytes) === */
    uint8_t state;                /**< v4_task_state_t */
    uint8_t priority;             /**< Priority (0=lowest, 255=highest) */
    uint32_t sleep_until_tick;    /**< Sleep end time (tick) */

    /* === Stack Size Configuration (4 bytes) === */
    uint8_t ds_size;  /**< DS size (number of elements) */
    uint8_t rs_size;  /**< RS size (number of elements) */

    /* === Statistics & Debug (4 bytes) === */
    uint16_t exec_count;  /**< Execution count (for debugging) */
    uint8_t reserved[2];  /**< Reserved for future use */
  } v4_task_t;

  /**
   * @brief Task Scheduler
   *
   * Manages all tasks and scheduling policy.
   */
  typedef struct
  {
    v4_task_t tasks[V4_MAX_TASKS];  /**< Task table (256 bytes) */

    uint8_t current_task;   /**< Currently running task ID */
    uint8_t task_count;     /**< Number of active tasks */
    uint32_t tick_count;    /**< System tick counter (ms) */
    uint32_t time_slice_ms; /**< Time slice (ms) */

    /* === Statistics (for debugging) === */
    uint32_t context_switches; /**< Context switch count */
    uint32_t preemptions;      /**< Preemption count */

    /* === Critical Section Management === */
    uint8_t critical_nesting; /**< Nesting depth */
    uint8_t reserved[3];      /**< Alignment */
  } v4_scheduler_t;

  /**
   * @brief Inter-task Message
   */
  typedef struct
  {
    uint8_t src_task;  /**< Source task ID */
    uint8_t dst_task;  /**< Destination task ID (0xFF=broadcast) */
    uint8_t msg_type;  /**< Message type (user-defined) */
    uint8_t flags;     /**< Flags (reserved) */
    int32_t data;      /**< Data payload (32bit) */
  } v4_message_t;

  /**
   * @brief Message Queue (Ring Buffer)
   */
  typedef struct
  {
    v4_message_t queue[V4_MSG_QUEUE_SIZE]; /**< Ring buffer (128 bytes) */
    uint8_t read_idx;                       /**< Read index */
    uint8_t write_idx;                      /**< Write index */
    uint8_t count;                          /**< Message count */
    uint8_t reserved;                       /**< Alignment */
  } v4_msg_queue_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
