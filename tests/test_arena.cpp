#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstdint>
#include <cstring>

#include "doctest.h"
#include "v4/arena.h"

TEST_CASE("Arena initialization")
{
  uint8_t buffer[1024];
  V4Arena arena;

  v4_arena_init(&arena, buffer, sizeof(buffer));

  CHECK(arena.buffer == buffer);
  CHECK(arena.size == 1024);
  CHECK(arena.used == 0);
  CHECK(v4_arena_used(&arena) == 0);
  CHECK(v4_arena_available(&arena) == 1024);
}

TEST_CASE("Arena basic allocation")
{
  uint8_t buffer[1024];
  V4Arena arena;
  v4_arena_init(&arena, buffer, sizeof(buffer));

  // Allocate 32 bytes with 1-byte alignment
  void* ptr1 = v4_arena_alloc(&arena, 32, 1);
  REQUIRE(ptr1 != nullptr);
  CHECK(ptr1 == buffer);
  CHECK(v4_arena_used(&arena) == 32);

  // Allocate another 64 bytes
  void* ptr2 = v4_arena_alloc(&arena, 64, 1);
  REQUIRE(ptr2 != nullptr);
  CHECK(ptr2 == buffer + 32);
  CHECK(v4_arena_used(&arena) == 96);
}

TEST_CASE("Arena alignment")
{
  alignas(16) uint8_t buffer[1024];
  V4Arena arena;
  v4_arena_init(&arena, buffer, sizeof(buffer));

  // Allocate 1 byte with 1-byte alignment
  void* ptr1 = v4_arena_alloc(&arena, 1, 1);
  REQUIRE(ptr1 != nullptr);
  CHECK(v4_arena_used(&arena) == 1);

  // Allocate 4 bytes with 4-byte alignment
  void* ptr2 = v4_arena_alloc(&arena, 4, 4);
  REQUIRE(ptr2 != nullptr);

  // Should be aligned to 4 bytes (next multiple of 4 after 1 is 4)
  CHECK(((uintptr_t)ptr2 & 3) == 0);
  CHECK(v4_arena_used(&arena) == 8);  // 4 (aligned start) + 4 (size)

  // Allocate 1 byte with 16-byte alignment
  void* ptr3 = v4_arena_alloc(&arena, 1, 16);
  REQUIRE(ptr3 != nullptr);
  CHECK(((uintptr_t)ptr3 & 15) == 0);
  CHECK(v4_arena_used(&arena) == 17);  // 16 (aligned start) + 1 (size)
}

TEST_CASE("Arena out of memory")
{
  uint8_t buffer[64];
  V4Arena arena;
  v4_arena_init(&arena, buffer, sizeof(buffer));

  // Allocate 60 bytes
  void* ptr1 = v4_arena_alloc(&arena, 60, 1);
  REQUIRE(ptr1 != nullptr);

  // Try to allocate 10 more bytes (should fail)
  void* ptr2 = v4_arena_alloc(&arena, 10, 1);
  CHECK(ptr2 == nullptr);

  // Should still have 60 bytes used
  CHECK(v4_arena_used(&arena) == 60);
}

TEST_CASE("Arena reset")
{
  uint8_t buffer[1024];
  V4Arena arena;
  v4_arena_init(&arena, buffer, sizeof(buffer));

  // Allocate some memory
  void* ptr1 = v4_arena_alloc(&arena, 100, 1);
  void* ptr2 = v4_arena_alloc(&arena, 200, 1);
  REQUIRE(ptr1 != nullptr);
  REQUIRE(ptr2 != nullptr);
  CHECK(v4_arena_used(&arena) == 300);

  // Reset
  v4_arena_reset(&arena);
  CHECK(v4_arena_used(&arena) == 0);
  CHECK(v4_arena_available(&arena) == 1024);

  // Should be able to allocate again from the beginning
  void* ptr3 = v4_arena_alloc(&arena, 50, 1);
  REQUIRE(ptr3 != nullptr);
  CHECK(ptr3 == buffer);
}

TEST_CASE("Arena edge cases")
{
  uint8_t buffer[64];
  V4Arena arena;

  SUBCASE("NULL arena init")
  {
    v4_arena_init(nullptr, buffer, 64);
    // Should not crash
  }

  SUBCASE("NULL arena alloc")
  {
    void* ptr = v4_arena_alloc(nullptr, 10, 1);
    CHECK(ptr == nullptr);
  }

  SUBCASE("Zero size alloc")
  {
    v4_arena_init(&arena, buffer, sizeof(buffer));
    void* ptr = v4_arena_alloc(&arena, 0, 1);
    CHECK(ptr == nullptr);
  }

  SUBCASE("Invalid alignment (not power of 2)")
  {
    v4_arena_init(&arena, buffer, sizeof(buffer));
    void* ptr = v4_arena_alloc(&arena, 10, 3);  // 3 is not power of 2
    CHECK(ptr == nullptr);
  }

  SUBCASE("Zero alignment")
  {
    v4_arena_init(&arena, buffer, sizeof(buffer));
    void* ptr = v4_arena_alloc(&arena, 10, 0);
    CHECK(ptr == nullptr);
  }
}

TEST_CASE("Arena usage tracking")
{
  uint8_t buffer[256];
  V4Arena arena;
  v4_arena_init(&arena, buffer, sizeof(buffer));

  CHECK(v4_arena_used(&arena) == 0);
  CHECK(v4_arena_available(&arena) == 256);

  v4_arena_alloc(&arena, 50, 1);
  CHECK(v4_arena_used(&arena) == 50);
  CHECK(v4_arena_available(&arena) == 206);

  v4_arena_alloc(&arena, 100, 1);
  CHECK(v4_arena_used(&arena) == 150);
  CHECK(v4_arena_available(&arena) == 106);

  v4_arena_reset(&arena);
  CHECK(v4_arena_used(&arena) == 0);
  CHECK(v4_arena_available(&arena) == 256);
}

TEST_CASE("Arena with struct allocation")
{
  struct TestStruct
  {
    int32_t a;
    int32_t b;
    int32_t c;
  };

  uint8_t buffer[1024];
  V4Arena arena;
  v4_arena_init(&arena, buffer, sizeof(buffer));

  // Allocate struct with proper alignment
  TestStruct* s1 =
      (TestStruct*)v4_arena_alloc(&arena, sizeof(TestStruct), alignof(TestStruct));
  REQUIRE(s1 != nullptr);
  CHECK(((uintptr_t)s1 & (alignof(TestStruct) - 1)) == 0);

  // Write to struct
  s1->a = 10;
  s1->b = 20;
  s1->c = 30;

  CHECK(s1->a == 10);
  CHECK(s1->b == 20);
  CHECK(s1->c == 30);

  // Allocate another struct
  TestStruct* s2 =
      (TestStruct*)v4_arena_alloc(&arena, sizeof(TestStruct), alignof(TestStruct));
  REQUIRE(s2 != nullptr);
  CHECK(s2 != s1);

  s2->a = 100;
  CHECK(s2->a == 100);
  CHECK(s1->a == 10);  // First struct unchanged
}
