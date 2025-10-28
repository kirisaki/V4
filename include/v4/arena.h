#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @file arena.h
   * @brief Simple linear memory allocator (bump allocator)
   *
   * Arena allocator for embedded systems where dynamic memory allocation
   * is restricted. Allocates from a fixed buffer without individual free().
   * Call v4_arena_reset() to free all allocations at once.
   *
   * Features:
   * - No fragmentation
   * - Fast allocation (O(1))
   * - Alignment support
   * - No individual free (reset only)
   */

  /**
   * @brief Arena allocator structure
   *
   * Manages a fixed buffer for linear memory allocation.
   */
  typedef struct V4Arena
  {
    uint8_t *buffer; /**< Managed buffer base pointer */
    size_t size;     /**< Total buffer size in bytes */
    size_t used;     /**< Currently used bytes */
  } V4Arena;

  /**
   * @brief Initialize an arena allocator
   *
   * @param arena  Pointer to arena structure
   * @param buffer Pointer to memory buffer to manage
   * @param size   Size of buffer in bytes
   */
  void v4_arena_init(V4Arena *arena, uint8_t *buffer, size_t size);

  /**
   * @brief Allocate memory from arena with alignment
   *
   * Allocates memory with specified alignment (must be power of 2).
   * Returns NULL if insufficient space.
   *
   * @param arena Pointer to arena
   * @param bytes Number of bytes to allocate
   * @param align Alignment requirement (must be power of 2)
   * @return Pointer to allocated memory, or NULL on failure
   */
  void *v4_arena_alloc(V4Arena *arena, size_t bytes, size_t align);

  /**
   * @brief Reset arena to initial state
   *
   * Frees all allocations at once by resetting used counter to 0.
   * Does not clear memory contents.
   *
   * @param arena Pointer to arena
   */
  void v4_arena_reset(V4Arena *arena);

  /**
   * @brief Get current used bytes
   *
   * @param arena Pointer to arena
   * @return Number of bytes currently allocated
   */
  size_t v4_arena_used(const V4Arena *arena);

  /**
   * @brief Get available bytes
   *
   * @param arena Pointer to arena
   * @return Number of bytes available for allocation
   */
  size_t v4_arena_available(const V4Arena *arena);

#ifdef __cplusplus
}
#endif
