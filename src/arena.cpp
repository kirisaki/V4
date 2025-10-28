#include "v4/arena.h"

#include <cstring>

extern "C" void v4_arena_init(V4Arena* arena, uint8_t* buffer, size_t size)
{
  if (!arena)
    return;

  arena->buffer = buffer;
  arena->size = size;
  arena->used = 0;
}

extern "C" void* v4_arena_alloc(V4Arena* arena, size_t bytes, size_t align)
{
  if (!arena || !arena->buffer || bytes == 0)
    return nullptr;

  // Alignment must be power of 2
  if (align == 0 || (align & (align - 1)) != 0)
    return nullptr;

  // Calculate aligned start position
  size_t current = arena->used;
  size_t aligned = (current + align - 1) & ~(align - 1);

  // Check if we have enough space
  if (aligned + bytes > arena->size)
    return nullptr;

  // Allocate
  void* ptr = arena->buffer + aligned;
  arena->used = aligned + bytes;

  return ptr;
}

extern "C" void v4_arena_reset(V4Arena* arena)
{
  if (!arena)
    return;

  arena->used = 0;
}

extern "C" size_t v4_arena_used(const V4Arena* arena)
{
  if (!arena)
    return 0;

  return arena->used;
}

extern "C" size_t v4_arena_available(const V4Arena* arena)
{
  if (!arena)
    return 0;

  return arena->size - arena->used;
}
