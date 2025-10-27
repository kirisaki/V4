#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "v4/errors.hpp"
#include "v4/internal/memory.hpp"
#include "v4/internal/vm.h"
#include "v4/opcodes.hpp"
#include "v4/vm_api.h"

extern "C" int v4_vm_version(void)
{
  return 0;
}

extern "C" void vm_reset(Vm* vm)
{
  // Reset data/return stacks to initial positions.
  vm->sp = vm->DS;
  vm->rp = vm->RS;
  vm->word_count = 0;
}

/* ========================== Data stack helpers =========================== */

static inline v4_err ds_push(Vm* vm, v4_i32 v)
{
  if (vm->sp >= vm->DS + 256)
    return static_cast<v4_err>(Err::StackOverflow);
  *vm->sp++ = v;
  return static_cast<v4_err>(Err::OK);
}

static inline v4_err ds_pop(Vm* vm, v4_i32* out)
{
  if (vm->sp <= vm->DS)
    return static_cast<v4_err>(Err::StackUnderflow);
  *out = *--vm->sp;
  return static_cast<v4_err>(Err::OK);
}

static inline v4_err ds_peek(const Vm* vm, int i, v4_i32* out)
{
  if (vm->sp - 1 - i < vm->DS)
    return static_cast<v4_err>(Err::StackUnderflow);
  *out = *(vm->sp - 1 - i);
  return static_cast<v4_err>(Err::OK);
}

static inline v4_err ds_poke(Vm* vm, int i, v4_i32 v)
{
  if (vm->sp - 1 - i < vm->DS)
    return static_cast<v4_err>(Err::StackUnderflow);
  *(vm->sp - 1 - i) = v;
  return static_cast<v4_err>(Err::OK);
}

/* ========================== Return stack helpers ========================= */

static inline v4_err rs_push(Vm* vm, v4_i32 v)
{
  if (vm->rp >= vm->RS + 64)
    return static_cast<v4_err>(Err::StackOverflow);
  *vm->rp++ = v;
  return static_cast<v4_err>(Err::OK);
}

static inline v4_err rs_pop(Vm* vm, v4_i32* out)
{
  if (vm->rp <= vm->RS)
    return static_cast<v4_err>(Err::StackUnderflow);
  *out = *--vm->rp;
  return static_cast<v4_err>(Err::OK);
}

static inline v4_err rs_peek(const Vm* vm, int i, v4_i32* out)
{
  if (vm->rp - 1 - i < vm->RS)
    return static_cast<v4_err>(Err::StackUnderflow);
  *out = *(vm->rp - 1 - i);
  return static_cast<v4_err>(Err::OK);
}

/* ===================== Little-endian readers/writers ===================== */

static inline v4_i32 read_i32_le(const v4_u8* p)
{
  return (v4_i32)((v4_u32)p[0] | ((uint32_t)p[1] << 8) | ((v4_u32)p[2] << 16) |
                  ((v4_i32)p[3] << 24));
}

static inline int16_t read_i16_le(const v4_u8* p)
{
  return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* =================== Internal raw bytecode interpreter =================== */

extern "C" v4_err vm_exec_raw(Vm* vm, const v4_u8* bc, int len)
{
  assert(vm && bc && len > 0);
  const v4_u8* ip = bc;
  const v4_u8* ip_end = bc + len;

  while (ip < ip_end)
  {
    v4::Op op = static_cast<v4::Op>(*ip++);
    switch (op)
    {
      /* -------- Literal -------- */
      case v4::Op::LIT:
      {
        if (ip + 4 > ip_end)
          return static_cast<v4_err>(Err::TruncatedLiteral);
        v4_i32 k = read_i32_le(ip);
        ip += 4;
        if (v4_err e = ds_push(vm, k))
          return e;
        break;
      }

      /* -------- Stack manipulation -------- */
      case v4::Op::DUP:
      {
        v4_i32 a;
        if (v4_err e = ds_peek(vm, 0, &a))
          return e;
        if (v4_err e = ds_push(vm, a))
          return e;
        break;
      }

      case v4::Op::DROP:
      {
        v4_i32 dummy;
        if (v4_err e = ds_pop(vm, &dummy))
          return e;
        break;
      }

      case v4::Op::SWAP:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_push(vm, a))
          return e;
        if (v4_err e = ds_push(vm, b))
          return e;
        break;
      }

      case v4::Op::OVER:
      {
        v4_i32 v;
        if (v4_err e = ds_peek(vm, 1, &v))
          return e;
        if (v4_err e = ds_push(vm, v))
          return e;
        break;
      }

      /* -------- Arithmetic -------- */
      case v4::Op::ADD:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_push(vm, b + a))
          return e;
        break;
      }

      case v4::Op::SUB:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_push(vm, b - a))
          return e;
        break;
      }

      case v4::Op::MUL:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a * b))
          return e;
        break;
      }

      case v4::Op::DIV:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (b == 0)
          return static_cast<v4_err>(Err::DivByZero);
        if (v4_err e = ds_push(vm, a / b))
          return e;
        break;
      }

      case v4::Op::MOD:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (b == 0)
          return static_cast<v4_err>(Err::DivByZero);
        if (v4_err e = ds_push(vm, a % b))
          return e;
        break;
      }

      /* -------- Comparison -------- */
      case v4::Op::EQ:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a == b ? V4_TRUE : V4_FALSE))
          return e;
        break;
      }

      case v4::Op::NE:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a != b ? V4_TRUE : V4_FALSE))
          return e;
        break;
      }

      case v4::Op::LT:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a < b ? V4_TRUE : V4_FALSE))
          return e;
        break;
      }

      case v4::Op::LE:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a <= b ? V4_TRUE : V4_FALSE))
          return e;
        break;
      }

      case v4::Op::GT:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a > b ? V4_TRUE : V4_FALSE))
          return e;
        break;
      }

      case v4::Op::GE:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a >= b ? V4_TRUE : V4_FALSE))
          return e;
        break;
      }

      /* -------- Bitwise -------- */
      case v4::Op::AND:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a & b))
          return e;
        break;
      }

      case v4::Op::OR:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a | b))
          return e;
        break;
      }

      case v4::Op::XOR:
      {
        v4_i32 a, b;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a ^ b))
          return e;
        break;
      }

      case v4::Op::INVERT:
      {
        v4_i32 a;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, ~a))
          return e;
        break;
      }

      /* -------- Control flow -------- */
      case v4::Op::JMP:
      {
        if (ip + 2 > ip_end)
          return static_cast<v4_err>(Err::TruncatedJump);
        int16_t off = read_i16_le(ip);
        ip += 2;
        const v4_u8* tgt = ip + off;
        if (tgt < bc || tgt > ip_end)
          return static_cast<v4_err>(Err::JumpOutOfRange);
        ip = tgt;
        break;
      }

      case v4::Op::JZ:
      {
        if (ip + 2 > ip_end)
          return static_cast<v4_err>(Err::TruncatedJump);
        int16_t off = read_i16_le(ip);
        ip += 2;
        v4_i32 cond;
        if (v4_err e = ds_pop(vm, &cond))
          return e;
        if (cond == 0)
        {
          const v4_u8* tgt = ip + off;
          if (tgt < bc || tgt > ip_end)
            return static_cast<v4_err>(Err::JumpOutOfRange);
          ip = tgt;
        }
        break;
      }

      case v4::Op::JNZ:
      {
        if (ip + 2 > ip_end)
          return static_cast<v4_err>(Err::TruncatedJump);
        int16_t off = read_i16_le(ip);
        ip += 2;
        v4_i32 cond;
        if (v4_err e = ds_pop(vm, &cond))
          return e;
        if (cond != 0)
        {
          const v4_u8* tgt = ip + off;
          if (tgt < bc || tgt > ip_end)
            return static_cast<v4_err>(Err::JumpOutOfRange);
          ip = tgt;
        }
        break;
      }

      /* -------- Memory -------- */
      case v4::Op::LOAD:
      {
        v4_i32 addr_i32;
        if (v4_err e = ds_pop(vm, &addr_i32))
          return e;
        v4_u32 addr = (v4_u32)addr_i32;
        v4_u32 val = 0;
        if (v4_err e = v4_mem_read32_core(vm, addr, &val))
          return e;
        if (v4_err e = ds_push(vm, (v4_i32)val))
          return e;
        break;
      }

      case v4::Op::STORE:
      {
        v4_i32 addr_i32, val_i32;
        if (v4_err e = ds_pop(vm, &addr_i32))
          return e;
        if (v4_err e = ds_pop(vm, &val_i32))
          return e;
        v4_u32 addr = (v4_u32)addr_i32;
        v4_u32 val = (v4_u32)val_i32;
        if (v4_err e = v4_mem_write32_core(vm, addr, val))
          return e;
        break;
      }

      /* -------- Return stack operations -------- */
      case v4::Op::TOR:
      {
        v4_i32 val;
        if (v4_err e = ds_pop(vm, &val))
          return e;
        if (v4_err e = rs_push(vm, val))
          return e;
        break;
      }

      case v4::Op::FROMR:
      {
        v4_i32 val;
        if (v4_err e = rs_pop(vm, &val))
          return e;
        if (v4_err e = ds_push(vm, val))
          return e;
        break;
      }

      case v4::Op::RFETCH:
      {
        v4_i32 val;
        if (v4_err e = rs_peek(vm, 0, &val))
          return e;
        if (v4_err e = ds_push(vm, val))
          return e;
        break;
      }

      /* -------- Call / Return -------- */
      case v4::Op::CALL:
      {
        if (ip + 2 > ip_end)
          return static_cast<v4_err>(Err::TruncatedJump);
        uint16_t word_idx = (uint16_t)ip[0] | ((uint16_t)ip[1] << 8);
        ip += 2;

        if (word_idx >= (uint16_t)vm->word_count)
          return static_cast<v4_err>(Err::InvalidWordIdx);

        Word* word = &vm->words[word_idx];
        if (!word->code || word->code_len <= 0)
          return static_cast<v4_err>(Err::InvalidArg);

        // Execute the called word
        if (v4_err e = vm_exec_raw(vm, word->code, word->code_len))
          return e;
        break;
      }

      case v4::Op::RET:
        return static_cast<v4_err>(Err::OK);

      default:
        return static_cast<v4_err>(Err::UnknownOp);
    }
  }

  return static_cast<v4_err>(Err::FellOffEnd);
}

/* ======================= Word management API ============================= */

extern "C" int vm_register_word(Vm* vm, const uint8_t* code, int code_len)
{
  if (!vm || !code || code_len <= 0)
    return static_cast<v4_err>(Err::InvalidArg);

  if (vm->word_count >= Vm::V4_MAX_WORDS)
    return static_cast<v4_err>(Err::DictionaryFull);

  int idx = vm->word_count;
  vm->words[idx].code = code;
  vm->words[idx].code_len = code_len;
  vm->word_count++;

  return idx;
}

extern "C" Word* vm_get_word(Vm* vm, int idx)
{
  if (!vm || idx < 0 || idx >= vm->word_count)
    return nullptr;
  return &vm->words[idx];
}

/* =========================== Public API entry ============================ */

extern "C" v4_err vm_exec(Vm* vm, Word* entry)
{
  if (!vm || !entry || !entry->code)
    return static_cast<v4_err>(Err::InvalidArg);

  return vm_exec_raw(vm, entry->code, entry->code_len);
}