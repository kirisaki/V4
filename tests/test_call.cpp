#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstring>

#include "doctest.h"
#include "v4/errors.hpp"
#include "v4/internal/vm.h"
#include "v4/opcodes.hpp"
#include "v4/vm_api.h"

using Op = v4::Op;

// Helper to emit bytecode
static void emit8(v4_u8* buf, int* pos, v4_u8 byte)
{
  buf[(*pos)++] = byte;
}

static void emit16(v4_u8* buf, int* pos, uint16_t val)
{
  buf[(*pos)++] = (v4_u8)(val & 0xFF);
  buf[(*pos)++] = (v4_u8)((val >> 8) & 0xFF);
}

static void emit32(v4_u8* buf, int* pos, v4_i32 val)
{
  buf[(*pos)++] = (v4_u8)(val & 0xFF);
  buf[(*pos)++] = (v4_u8)((val >> 8) & 0xFF);
  buf[(*pos)++] = (v4_u8)((val >> 16) & 0xFF);
  buf[(*pos)++] = (v4_u8)((val >> 24) & 0xFF);
}

/* ------------------------------------------------------------------------- */
/* Word registration API tests                                               */
/* ------------------------------------------------------------------------- */
TEST_CASE("vm_register_word - basic registration")
{
  Vm vm{};
  vm_reset(&vm);

  // Simple word: LIT 42; RET
  v4_u8 word_code[16];
  int k = 0;
  emit8(word_code, &k, (v4_u8)Op::LIT);
  emit32(word_code, &k, 42);
  emit8(word_code, &k, (v4_u8)Op::RET);

  int idx = vm_register_word(&vm, nullptr, word_code, k);
  CHECK(idx == 0);  // First word should get index 0
  CHECK(vm.word_count == 1);
}

TEST_CASE("vm_register_word - multiple words")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 word1[16], word2[16], word3[16];
  int k = 0;

  // Word 1: LIT 10; RET
  k = 0;
  emit8(word1, &k, (v4_u8)Op::LIT);
  emit32(word1, &k, 10);
  emit8(word1, &k, (v4_u8)Op::RET);

  // Word 2: LIT 20; RET
  k = 0;
  emit8(word2, &k, (v4_u8)Op::LIT);
  emit32(word2, &k, 20);
  emit8(word2, &k, (v4_u8)Op::RET);

  // Word 3: LIT 30; RET
  k = 0;
  emit8(word3, &k, (v4_u8)Op::LIT);
  emit32(word3, &k, 30);
  emit8(word3, &k, (v4_u8)Op::RET);

  int idx1 = vm_register_word(&vm, nullptr, word1, 6);
  int idx2 = vm_register_word(&vm, nullptr, word2, 6);
  int idx3 = vm_register_word(&vm, nullptr, word3, 6);

  CHECK(idx1 == 0);
  CHECK(idx2 == 1);
  CHECK(idx3 == 2);
  CHECK(vm.word_count == 3);
}

TEST_CASE("vm_register_word - invalid arguments")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::RET);

  // NULL vm
  CHECK(vm_register_word(nullptr, nullptr, code, k) == static_cast<int>(Err::InvalidArg));

  // NULL code
  CHECK(vm_register_word(&vm, nullptr, nullptr, k) == static_cast<int>(Err::InvalidArg));

  // Invalid length
  CHECK(vm_register_word(&vm, nullptr, code, 0) == static_cast<int>(Err::InvalidArg));
  CHECK(vm_register_word(&vm, nullptr, code, -1) == static_cast<int>(Err::InvalidArg));
}

TEST_CASE("vm_register_word - dictionary full")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 word_code[16];
  int k = 0;
  emit8(word_code, &k, (v4_u8)Op::RET);

  // Fill up the dictionary (max 256 words)
  for (int i = 0; i < 256; i++)
  {
    int idx = vm_register_word(&vm, nullptr, word_code, k);
    CHECK(idx == i);
  }

  // Next registration should fail
  int idx = vm_register_word(&vm, nullptr, word_code, k);
  CHECK(idx == static_cast<int>(Err::DictionaryFull));
  CHECK(vm.word_count == 256);
}

/* ------------------------------------------------------------------------- */
/* vm_get_word API tests                                                     */
/* ------------------------------------------------------------------------- */
TEST_CASE("vm_get_word - retrieve registered word")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 word_code[16];
  int k = 0;
  emit8(word_code, &k, (v4_u8)Op::LIT);
  emit32(word_code, &k, 99);
  emit8(word_code, &k, (v4_u8)Op::RET);

  int idx = vm_register_word(&vm, nullptr, word_code, k);
  REQUIRE(idx == 0);

  Word* word = vm_get_word(&vm, idx);
  REQUIRE(word != nullptr);
  CHECK(word->code == word_code);
  CHECK(word->code_len == k);
}

TEST_CASE("vm_get_word - invalid index")
{
  Vm vm{};
  vm_reset(&vm);

  // No words registered yet
  CHECK(vm_get_word(&vm, 0) == nullptr);
  CHECK(vm_get_word(&vm, -1) == nullptr);
  CHECK(vm_get_word(&vm, 100) == nullptr);

  // Register one word
  v4_u8 word_code[16];
  int k = 0;
  emit8(word_code, &k, (v4_u8)Op::RET);
  vm_register_word(&vm, nullptr, word_code, k);

  // Valid index
  CHECK(vm_get_word(&vm, 0) != nullptr);

  // Invalid indices
  CHECK(vm_get_word(&vm, 1) == nullptr);
  CHECK(vm_get_word(&vm, -1) == nullptr);
}

TEST_CASE("vm_get_word - NULL vm")
{
  CHECK(vm_get_word(nullptr, 0) == nullptr);
}

/* ------------------------------------------------------------------------- */
/* CALL instruction tests                                                    */
/* ------------------------------------------------------------------------- */
TEST_CASE("CALL instruction - basic word call")
{
  Vm vm{};
  vm_reset(&vm);

  // Word: LIT 42; RET
  v4_u8 word_code[16];
  int wk = 0;
  emit8(word_code, &wk, (v4_u8)Op::LIT);
  emit32(word_code, &wk, 42);
  emit8(word_code, &wk, (v4_u8)Op::RET);

  int word_idx = vm_register_word(&vm, nullptr, word_code, wk);
  REQUIRE(word_idx == 0);

  // Main code: CALL 0; RET
  v4_u8 main_code[16];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)word_idx);
  emit8(main_code, &mk, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, main_code, mk);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 42);
}

TEST_CASE("CALL instruction - word with arithmetic")
{
  Vm vm{};
  vm_reset(&vm);

  // Word "double": DUP; ADD; RET  (2x the top value)
  v4_u8 word_code[16];
  int wk = 0;
  emit8(word_code, &wk, (v4_u8)Op::DUP);
  emit8(word_code, &wk, (v4_u8)Op::ADD);
  emit8(word_code, &wk, (v4_u8)Op::RET);

  int word_idx = vm_register_word(&vm, nullptr, word_code, wk);
  REQUIRE(word_idx == 0);

  // Main code: LIT 21; CALL 0; RET
  v4_u8 main_code[32];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::LIT);
  emit32(main_code, &mk, 21);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)word_idx);
  emit8(main_code, &mk, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, main_code, mk);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 42);
}

TEST_CASE("CALL instruction - multiple calls")
{
  Vm vm{};
  vm_reset(&vm);

  // Word "add10": LIT 10; ADD; RET
  v4_u8 word_code[16];
  int wk = 0;
  emit8(word_code, &wk, (v4_u8)Op::LIT);
  emit32(word_code, &wk, 10);
  emit8(word_code, &wk, (v4_u8)Op::ADD);
  emit8(word_code, &wk, (v4_u8)Op::RET);

  int word_idx = vm_register_word(&vm, nullptr, word_code, wk);
  REQUIRE(word_idx == 0);

  // Main code: LIT 5; CALL 0; CALL 0; CALL 0; RET
  // Result should be 5 + 10 + 10 + 10 = 35
  v4_u8 main_code[32];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::LIT);
  emit32(main_code, &mk, 5);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)word_idx);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)word_idx);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)word_idx);
  emit8(main_code, &mk, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, main_code, mk);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 35);
}

TEST_CASE("CALL instruction - calling multiple different words")
{
  Vm vm{};
  vm_reset(&vm);

  // Word 0 "square": DUP; MUL; RET
  v4_u8 word0[16];
  int k0 = 0;
  emit8(word0, &k0, (v4_u8)Op::DUP);
  emit8(word0, &k0, (v4_u8)Op::MUL);
  emit8(word0, &k0, (v4_u8)Op::RET);

  // Word 1 "increment": LIT 1; ADD; RET
  v4_u8 word1[16];
  int k1 = 0;
  emit8(word1, &k1, (v4_u8)Op::LIT);
  emit32(word1, &k1, 1);
  emit8(word1, &k1, (v4_u8)Op::ADD);
  emit8(word1, &k1, (v4_u8)Op::RET);

  int idx0 = vm_register_word(&vm, nullptr, word0, k0);
  int idx1 = vm_register_word(&vm, nullptr, word1, k1);
  REQUIRE(idx0 == 0);
  REQUIRE(idx1 == 1);

  // Main: LIT 5; CALL 0 (square); CALL 1 (increment); RET
  // Result: 5^2 + 1 = 26
  v4_u8 main_code[32];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::LIT);
  emit32(main_code, &mk, 5);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)idx0);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)idx1);
  emit8(main_code, &mk, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, main_code, mk);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 26);
}

TEST_CASE("CALL instruction - nested calls")
{
  Vm vm{};
  vm_reset(&vm);

  // Word 0 "add5": LIT 5; ADD; RET
  v4_u8 word0[16];
  int k0 = 0;
  emit8(word0, &k0, (v4_u8)Op::LIT);
  emit32(word0, &k0, 5);
  emit8(word0, &k0, (v4_u8)Op::ADD);
  emit8(word0, &k0, (v4_u8)Op::RET);

  int idx0 = vm_register_word(&vm, nullptr, word0, k0);
  REQUIRE(idx0 == 0);

  // Word 1 "add15": CALL 0; CALL 0; CALL 0; RET  (calls add5 three times)
  v4_u8 word1[32];
  int k1 = 0;
  emit8(word1, &k1, (v4_u8)Op::CALL);
  emit16(word1, &k1, (uint16_t)idx0);
  emit8(word1, &k1, (v4_u8)Op::CALL);
  emit16(word1, &k1, (uint16_t)idx0);
  emit8(word1, &k1, (v4_u8)Op::CALL);
  emit16(word1, &k1, (uint16_t)idx0);
  emit8(word1, &k1, (v4_u8)Op::RET);

  int idx1 = vm_register_word(&vm, nullptr, word1, k1);
  REQUIRE(idx1 == 1);

  // Main: LIT 10; CALL 1; RET
  // Result: 10 + 5 + 5 + 5 = 25
  v4_u8 main_code[32];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::LIT);
  emit32(main_code, &mk, 10);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)idx1);
  emit8(main_code, &mk, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, main_code, mk);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 25);
}

TEST_CASE("CALL instruction - invalid word index")
{
  Vm vm{};
  vm_reset(&vm);

  // No words registered, try to call word 0
  v4_u8 main_code[16];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)0);
  emit8(main_code, &mk, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, main_code, mk);
  CHECK(rc == static_cast<int>(Err::InvalidWordIdx));
}

TEST_CASE("CALL instruction - out of bounds word index")
{
  Vm vm{};
  vm_reset(&vm);

  // Register one word
  v4_u8 word_code[16];
  int wk = 0;
  emit8(word_code, &wk, (v4_u8)Op::LIT);
  emit32(word_code, &wk, 42);
  emit8(word_code, &wk, (v4_u8)Op::RET);
  vm_register_word(&vm, nullptr, word_code, wk);

  // Try to call word 5 (doesn't exist)
  v4_u8 main_code[16];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)5);
  emit8(main_code, &mk, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, main_code, mk);
  CHECK(rc == static_cast<int>(Err::InvalidWordIdx));
}

TEST_CASE("CALL instruction - truncated instruction")
{
  Vm vm{};
  vm_reset(&vm);

  // Register a word
  v4_u8 word_code[16];
  int wk = 0;
  emit8(word_code, &wk, (v4_u8)Op::RET);
  vm_register_word(&vm, nullptr, word_code, wk);

  // Main code with truncated CALL (missing 1 byte)
  v4_u8 main_code[16];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit8(main_code, &mk, 0);  // Only 1 byte instead of 2

  int rc = vm_exec_raw(&vm, main_code, mk);
  CHECK(rc == static_cast<int>(Err::TruncatedJump));
}

/* ------------------------------------------------------------------------- */
/* vm_exec public API tests                                                  */
/* ------------------------------------------------------------------------- */
TEST_CASE("vm_exec - execute word via public API")
{
  Vm vm{};
  vm_reset(&vm);

  // Word: LIT 100; LIT 200; ADD; RET
  v4_u8 word_code[32];
  int wk = 0;
  emit8(word_code, &wk, (v4_u8)Op::LIT);
  emit32(word_code, &wk, 100);
  emit8(word_code, &wk, (v4_u8)Op::LIT);
  emit32(word_code, &wk, 200);
  emit8(word_code, &wk, (v4_u8)Op::ADD);
  emit8(word_code, &wk, (v4_u8)Op::RET);

  Word entry;
  entry.code = word_code;
  entry.code_len = wk;

  v4_err rc = vm_exec(&vm, &entry);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 300);
}

TEST_CASE("vm_exec - invalid arguments")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::RET);

  Word entry;
  entry.code = code;
  entry.code_len = k;

  // NULL vm
  CHECK(vm_exec(nullptr, &entry) == static_cast<v4_err>(Err::InvalidArg));

  // NULL entry
  CHECK(vm_exec(&vm, nullptr) == static_cast<v4_err>(Err::InvalidArg));

  // NULL code in entry
  entry.code = nullptr;
  CHECK(vm_exec(&vm, &entry) == static_cast<v4_err>(Err::InvalidArg));
}

TEST_CASE("vm_exec - word with error propagation")
{
  Vm vm{};
  vm_reset(&vm);

  // Word that divides by zero: LIT 10; LIT 0; DIV; RET
  v4_u8 word_code[32];
  int wk = 0;
  emit8(word_code, &wk, (v4_u8)Op::LIT);
  emit32(word_code, &wk, 10);
  emit8(word_code, &wk, (v4_u8)Op::LIT);
  emit32(word_code, &wk, 0);
  emit8(word_code, &wk, (v4_u8)Op::DIV);
  emit8(word_code, &wk, (v4_u8)Op::RET);

  Word entry;
  entry.code = word_code;
  entry.code_len = wk;

  v4_err rc = vm_exec(&vm, &entry);
  CHECK(rc == static_cast<v4_err>(Err::DivByZero));
}

TEST_CASE("vm_exec - word calling other words")
{
  Vm vm{};
  vm_reset(&vm);

  // Register helper word: LIT 10; ADD; RET
  v4_u8 helper_code[16];
  int hk = 0;
  emit8(helper_code, &hk, (v4_u8)Op::LIT);
  emit32(helper_code, &hk, 10);
  emit8(helper_code, &hk, (v4_u8)Op::ADD);
  emit8(helper_code, &hk, (v4_u8)Op::RET);

  int helper_idx = vm_register_word(&vm, nullptr, helper_code, hk);
  REQUIRE(helper_idx == 0);

  // Main word: LIT 5; CALL 0; CALL 0; RET
  // Result: 5 + 10 + 10 = 25
  v4_u8 main_code[32];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::LIT);
  emit32(main_code, &mk, 5);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)helper_idx);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)helper_idx);
  emit8(main_code, &mk, (v4_u8)Op::RET);

  Word entry;
  entry.code = main_code;
  entry.code_len = mk;

  v4_err rc = vm_exec(&vm, &entry);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 25);
}

TEST_CASE("vm_reset clears word dictionary")
{
  Vm vm{};
  vm_reset(&vm);

  // Register some words
  v4_u8 word_code[16];
  int k = 0;
  emit8(word_code, &k, (v4_u8)Op::RET);

  vm_register_word(&vm, nullptr, word_code, k);
  vm_register_word(&vm, nullptr, word_code, k);
  vm_register_word(&vm, nullptr, word_code, k);

  CHECK(vm.word_count == 3);

  // Reset should clear the dictionary
  vm_reset(&vm);
  CHECK(vm.word_count == 0);
}

/* ------------------------------------------------------------------------- */
/* Word name tests                                                           */
/* ------------------------------------------------------------------------- */
TEST_CASE("vm_register_word - named word")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::RET);

  int idx = vm_register_word(&vm, "TEST", code, k);
  REQUIRE(idx >= 0);
  REQUIRE(vm.words[idx].name != nullptr);
  CHECK(strcmp(vm.words[idx].name, "TEST") == 0);

  vm_reset(&vm);  // Free allocated name
}

TEST_CASE("vm_register_word - anonymous word (NULL name)")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::RET);

  int idx = vm_register_word(&vm, nullptr, code, k);
  REQUIRE(idx >= 0);
  CHECK(vm.words[idx].name == nullptr);
}

TEST_CASE("vm_register_word - multiple named words")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 code1[16], code2[16], code3[16];
  int k = 0;

  emit8(code1, &k, (v4_u8)Op::RET);
  emit8(code2, &k, (v4_u8)Op::RET);
  emit8(code3, &k, (v4_u8)Op::RET);

  int idx1 = vm_register_word(&vm, "DOUBLE", code1, k);
  int idx2 = vm_register_word(&vm, "SQUARE", code2, k);
  int idx3 = vm_register_word(&vm, nullptr, code3, k);  // anonymous

  REQUIRE(idx1 >= 0);
  REQUIRE(idx2 >= 0);
  REQUIRE(idx3 >= 0);

  CHECK(vm.words[idx1].name != nullptr);
  CHECK(strcmp(vm.words[idx1].name, "DOUBLE") == 0);

  CHECK(vm.words[idx2].name != nullptr);
  CHECK(strcmp(vm.words[idx2].name, "SQUARE") == 0);

  CHECK(vm.words[idx3].name == nullptr);

  vm_reset(&vm);  // Free allocated names
}

TEST_CASE("vm_reset - frees word names")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::RET);

  // Register words with names
  vm_register_word(&vm, "WORD1", code, k);
  vm_register_word(&vm, "WORD2", code, k);
  vm_register_word(&vm, nullptr, code, k);  // anonymous

  CHECK(vm.word_count == 3);

  // Reset should free the names and clear the dictionary
  vm_reset(&vm);
  CHECK(vm.word_count == 0);
}

TEST_CASE("vm_register_word - name is copied (strdup)")
{
  Vm vm{};
  vm_reset(&vm);

  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::RET);

  char name_buffer[64];
  strcpy(name_buffer, "ORIGINAL");

  int idx = vm_register_word(&vm, name_buffer, code, k);
  REQUIRE(idx >= 0);
  REQUIRE(vm.words[idx].name != nullptr);
  CHECK(strcmp(vm.words[idx].name, "ORIGINAL") == 0);

  // Modify the original buffer - word name should be unchanged
  strcpy(name_buffer, "MODIFIED");
  CHECK(strcmp(vm.words[idx].name, "ORIGINAL") == 0);

  vm_reset(&vm);  // Free allocated name
}

TEST_CASE("CALL instruction - named word execution")
{
  Vm vm{};
  vm_reset(&vm);

  // Word "ADD10": LIT 10; ADD; RET
  v4_u8 word_code[16];
  int wk = 0;
  emit8(word_code, &wk, (v4_u8)Op::LIT);
  emit32(word_code, &wk, 10);
  emit8(word_code, &wk, (v4_u8)Op::ADD);
  emit8(word_code, &wk, (v4_u8)Op::RET);

  int word_idx = vm_register_word(&vm, "ADD10", word_code, wk);
  REQUIRE(word_idx >= 0);
  REQUIRE(vm.words[word_idx].name != nullptr);
  CHECK(strcmp(vm.words[word_idx].name, "ADD10") == 0);

  // Main code: LIT 5; CALL 0; RET
  v4_u8 main_code[32];
  int mk = 0;
  emit8(main_code, &mk, (v4_u8)Op::LIT);
  emit32(main_code, &mk, 5);
  emit8(main_code, &mk, (v4_u8)Op::CALL);
  emit16(main_code, &mk, (uint16_t)word_idx);
  emit8(main_code, &mk, (v4_u8)Op::RET);

  int rc = vm_exec_raw(&vm, main_code, mk);
  CHECK(rc == 0);
  CHECK(vm.sp == vm.DS + 1);
  CHECK(vm.DS[0] == 15);

  vm_reset(&vm);  // Free allocated name
}
TEST_CASE("vm_destroy - frees word names (memory leak test)")
{
  // This test verifies that vm_destroy properly frees word names
  // allocated by vm_register_word. Under AddressSanitizer, this would
  // fail if word names are not freed.

  uint8_t ram[64] = {0};
  VmConfig cfg = {ram, sizeof(ram), nullptr, 0};
  struct Vm* vm = vm_create(&cfg);
  REQUIRE(vm != nullptr);

  v4_u8 code[16];
  int k = 0;
  emit8(code, &k, (v4_u8)Op::RET);

  // Register multiple words with names - these will be strdup'd
  int idx1 = vm_register_word(vm, "WORD1", code, k);
  int idx2 = vm_register_word(vm, "WORD2", code, k);
  int idx3 = vm_register_word(vm, "WORD3", code, k);

  REQUIRE(idx1 >= 0);
  REQUIRE(idx2 >= 0);
  REQUIRE(idx3 >= 0);

  // vm_destroy must free all the strdup'd names
  vm_destroy(vm);

  // If vm_destroy doesn't free the names, AddressSanitizer will report:
  // "Direct leak of N byte(s) in M object(s) allocated from strdup"
}
