#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstring>

#include "doctest.h"
#include "v4/arena.h"
#include "v4/errors.hpp"
#include "v4/internal/vm.h"
#include "v4/opcodes.hpp"
#include "v4/vm_api.h"

/* Helper to emit bytecode */
static void emit8(v4_u8* code, int* k, v4_u8 byte)
{
  code[(*k)++] = byte;
}

TEST_CASE("VM with arena - word registration")
{
  // Setup arena
  uint8_t arena_buffer[1024];
  V4Arena arena;
  v4_arena_init(&arena, arena_buffer, sizeof(arena_buffer));

  // Create VM with arena
  uint8_t mem[256];
  VmConfig cfg;
  cfg.mem = mem;
  cfg.mem_size = sizeof(mem);
  cfg.mmio = nullptr;
  cfg.mmio_count = 0;
  cfg.arena = &arena;

  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);
  CHECK(vm->arena == &arena);

  // Register word with name
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  int idx = vm_register_word(vm, "TEST_WORD", code, k);
  CHECK(idx == 0);

  // Check word was registered
  Word* word = vm_get_word(vm, idx);
  REQUIRE(word != nullptr);
  REQUIRE(word->name != nullptr);
  CHECK(strcmp(word->name, "TEST_WORD") == 0);

  // Check name is in arena
  size_t arena_used = v4_arena_used(&arena);
  CHECK(arena_used > 0);
  CHECK(arena_used >= strlen("TEST_WORD") + 1);

  vm_destroy(vm);
}

TEST_CASE("VM with arena - multiple words")
{
  uint8_t arena_buffer[2048];
  V4Arena arena;
  v4_arena_init(&arena, arena_buffer, sizeof(arena_buffer));

  uint8_t mem[256];
  VmConfig cfg;
  cfg.mem = mem;
  cfg.mem_size = sizeof(mem);
  cfg.mmio = nullptr;
  cfg.mmio_count = 0;
  cfg.arena = &arena;

  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  // Register multiple words
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  vm_register_word(vm, "WORD1", code, k);
  vm_register_word(vm, "WORD2", code, k);
  vm_register_word(vm, "WORD3", code, k);

  CHECK(vm->word_count == 3);

  // Check all names are present
  CHECK(strcmp(vm->words[0].name, "WORD1") == 0);
  CHECK(strcmp(vm->words[1].name, "WORD2") == 0);
  CHECK(strcmp(vm->words[2].name, "WORD3") == 0);

  // Check arena usage
  size_t expected_min = strlen("WORD1") + 1 + strlen("WORD2") + 1 + strlen("WORD3") + 1;
  CHECK(v4_arena_used(&arena) >= expected_min);

  vm_destroy(vm);
}

TEST_CASE("VM with arena - anonymous words")
{
  uint8_t arena_buffer[1024];
  V4Arena arena;
  v4_arena_init(&arena, arena_buffer, sizeof(arena_buffer));

  uint8_t mem[256];
  VmConfig cfg;
  cfg.mem = mem;
  cfg.mem_size = sizeof(mem);
  cfg.mmio = nullptr;
  cfg.mmio_count = 0;
  cfg.arena = &arena;

  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  // Register anonymous word (no name)
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  size_t arena_before = v4_arena_used(&arena);
  int idx = vm_register_word(vm, nullptr, code, k);
  size_t arena_after = v4_arena_used(&arena);

  CHECK(idx == 0);
  CHECK(vm->words[0].name == nullptr);

  // Arena should not grow for anonymous word
  CHECK(arena_before == arena_after);

  vm_destroy(vm);
}

TEST_CASE("VM with arena - reset dictionary")
{
  uint8_t arena_buffer[1024];
  V4Arena arena;
  v4_arena_init(&arena, arena_buffer, sizeof(arena_buffer));

  uint8_t mem[256];
  VmConfig cfg;
  cfg.mem = mem;
  cfg.mem_size = sizeof(mem);
  cfg.mmio = nullptr;
  cfg.mmio_count = 0;
  cfg.arena = &arena;

  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  // Register words
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  vm_register_word(vm, "WORD1", code, k);
  vm_register_word(vm, "WORD2", code, k);

  size_t arena_used = v4_arena_used(&arena);
  CHECK(arena_used > 0);

  // Reset dictionary (should not free arena memory)
  vm_reset_dictionary(vm);

  CHECK(vm->word_count == 0);
  // Arena usage should remain the same (no free)
  CHECK(v4_arena_used(&arena) == arena_used);

  vm_destroy(vm);
}

TEST_CASE("VM with arena - arena reset and reuse")
{
  uint8_t arena_buffer[1024];
  V4Arena arena;
  v4_arena_init(&arena, arena_buffer, sizeof(arena_buffer));

  uint8_t mem[256];
  VmConfig cfg;
  cfg.mem = mem;
  cfg.mem_size = sizeof(mem);
  cfg.mmio = nullptr;
  cfg.mmio_count = 0;
  cfg.arena = &arena;

  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  // Register words
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  vm_register_word(vm, "WORD1", code, k);
  vm_register_word(vm, "WORD2", code, k);

  size_t arena_used = v4_arena_used(&arena);
  CHECK(arena_used > 0);

  // Reset dictionary
  vm_reset_dictionary(vm);

  // User manually resets arena
  v4_arena_reset(&arena);
  CHECK(v4_arena_used(&arena) == 0);

  // Register new words (should reuse arena space)
  vm_register_word(vm, "NEW_WORD1", code, k);
  vm_register_word(vm, "NEW_WORD2", code, k);

  CHECK(vm->word_count == 2);
  CHECK(strcmp(vm->words[0].name, "NEW_WORD1") == 0);
  CHECK(strcmp(vm->words[1].name, "NEW_WORD2") == 0);

  vm_destroy(vm);
}

TEST_CASE("VM without arena - uses malloc")
{
  // Create VM without arena (NULL)
  uint8_t mem[256];
  VmConfig cfg;
  cfg.mem = mem;
  cfg.mem_size = sizeof(mem);
  cfg.mmio = nullptr;
  cfg.mmio_count = 0;
  cfg.arena = nullptr;  // No arena

  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);
  CHECK(vm->arena == nullptr);

  // Register word with name
  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  int idx = vm_register_word(vm, "MALLOC_WORD", code, k);
  CHECK(idx == 0);

  // Check word was registered
  Word* word = vm_get_word(vm, idx);
  REQUIRE(word != nullptr);
  REQUIRE(word->name != nullptr);
  CHECK(strcmp(word->name, "MALLOC_WORD") == 0);

  // Reset dictionary (should free malloc'd memory)
  vm_reset_dictionary(vm);
  CHECK(vm->word_count == 0);

  vm_destroy(vm);
  // If this test passes without memory leaks in valgrind, malloc/free works correctly
}

TEST_CASE("VM with arena - out of memory")
{
  // Very small arena
  uint8_t arena_buffer[32];
  V4Arena arena;
  v4_arena_init(&arena, arena_buffer, sizeof(arena_buffer));

  uint8_t mem[256];
  VmConfig cfg;
  cfg.mem = mem;
  cfg.mem_size = sizeof(mem);
  cfg.mmio = nullptr;
  cfg.mmio_count = 0;
  cfg.arena = &arena;

  Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, static_cast<v4_u8>(v4::Op::RET));

  // Register words until arena runs out
  int idx1 = vm_register_word(vm, "SHORT", code, k);
  CHECK(idx1 == 0);

  // Try to register a word with a long name that won't fit
  int idx2 =
      vm_register_word(vm, "THIS_IS_A_VERY_LONG_WORD_NAME_THAT_WILL_NOT_FIT", code, k);
  CHECK(idx2 < 0);  // Should fail (InvalidArg)

  vm_destroy(vm);
}
