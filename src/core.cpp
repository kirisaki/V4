#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include "v4/vm_api.h"
#include "v4/opcodes.hpp"
#include "v4/errors.hpp"
#include "v4/internal/vm.h"
#include "v4/internal/memory.hpp"

extern "C" int v4_vm_version(void)
{
  return 0;
}

extern "C" void vm_reset(Vm *vm)
{
  // Reset data/return stacks to initial positions.
  vm->sp = vm->DS;
  vm->rp = vm->RS;
}

/* ========================== Data stack helpers =========================== */

static inline void ds_push(Vm *vm, int32_t v)
{
  assert(vm->sp < vm->DS + 256 && "Data stack overflow");
  *vm->sp++ = v;
}
static inline int32_t ds_pop(Vm *vm)
{
  assert(vm->sp > vm->DS && "Data stack underflow");
  return *--vm->sp;
}
static inline int32_t ds_peek(const Vm *vm, int i = 0)
{
  assert(vm->sp - 1 - i >= vm->DS && "Data stack peek underflow");
  return *(vm->sp - 1 - i);
}
static inline void ds_poke(Vm *vm, int i, int32_t v)
{
  assert(vm->sp - 1 - i >= vm->DS && "Data stack poke underflow");
  *(vm->sp - 1 - i) = v;
}

/* ===================== Little-endian readers/writers ===================== */

static inline int32_t read_i32_le(const uint8_t *p)
{
  return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                   ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}
static inline int16_t read_i16_le(const uint8_t *p)
{
  return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* =================== Internal raw bytecode interpreter =================== */
/* This keeps your current loop intact so existing tests keep running.
 * Public API vm_exec(...) will call into a Word-based path later.          */

extern "C" v4_err vm_exec_raw(Vm *vm, const uint8_t *bc, int len)
{
  assert(vm && bc && len > 0);
  const uint8_t *ip = bc;
  const uint8_t *ip_end = bc + len;

  while (ip < ip_end)
  {
    Op op = static_cast<Op>(*ip++);
    switch (op)
    {
    /* -------- Literal -------- */
    case Op::LIT:
    {
      if (ip + 4 > ip_end)
        return static_cast<v4_err>(Err::TruncatedLiteral);
      int32_t k = read_i32_le(ip);
      ip += 4;
      ds_push(vm, k);
      break;
    }

    /* ---- Stack manipulation ---- */
    case Op::DUP:
    {
      int32_t a = ds_peek(vm, 0);
      ds_push(vm, a);
      break;
    }
    case Op::DROP:
    {
      (void)ds_pop(vm);
      break;
    }
    case Op::SWAP:
    {
      int32_t a = ds_pop(vm);
      int32_t b = ds_pop(vm);
      ds_push(vm, a);
      ds_push(vm, b);
      break;
    }
    case Op::OVER:
    {
      int32_t v = ds_peek(vm, 1);
      ds_push(vm, v);
      break;
    }

    /* -------- Arithmetic -------- */
    case Op::ADD:
    {
      int32_t a = ds_pop(vm);
      int32_t b = ds_pop(vm);
      ds_push(vm, b + a);
      break;
    }
    case Op::SUB:
    {
      int32_t a = ds_pop(vm);
      int32_t b = ds_pop(vm);
      ds_push(vm, b - a);
      break;
    }
    case Op::MUL:
    {
      int32_t b = ds_pop(vm);
      int32_t a = ds_pop(vm);
      ds_push(vm, (int32_t)(a * b));
      break;
    }
    case Op::DIV:
    {
      int32_t b = ds_pop(vm);
      int32_t a = ds_pop(vm);
      if (b == 0)
        return static_cast<v4_err>(Err::DivByZero);
      ds_push(vm, a / b);
      break;
    }
    case Op::MOD:
    {
      int32_t b = ds_pop(vm);
      int32_t a = ds_pop(vm);
      if (b == 0)
        return static_cast<v4_err>(Err::DivByZero);
      ds_push(vm, a % b);
      break;
    }

    /* -------- Comparison (Forth truth: -1 = true, 0 = false) -------- */
    case Op::EQ:
    {
      int32_t b = ds_pop(vm), a = ds_pop(vm);
      ds_push(vm, a == b ? V4_TRUE : V4_FALSE);
      break;
    }
    case Op::NE:
    {
      int32_t b = ds_pop(vm), a = ds_pop(vm);
      ds_push(vm, a != b ? V4_TRUE : V4_FALSE);
      break;
    }
    case Op::LT:
    {
      int32_t b = ds_pop(vm), a = ds_pop(vm);
      ds_push(vm, a < b ? V4_TRUE : V4_FALSE);
      break;
    }
    case Op::LE:
    {
      int32_t b = ds_pop(vm), a = ds_pop(vm);
      ds_push(vm, a <= b ? V4_TRUE : V4_FALSE);
      break;
    }
    case Op::GT:
    {
      int32_t b = ds_pop(vm), a = ds_pop(vm);
      ds_push(vm, a > b ? V4_TRUE : V4_FALSE);
      break;
    }
    case Op::GE:
    {
      int32_t b = ds_pop(vm), a = ds_pop(vm);
      ds_push(vm, a >= b ? V4_TRUE : V4_FALSE);
      break;
    }

    /* -------- Bitwise -------- */
    case Op::AND:
    {
      int32_t b = ds_pop(vm), a = ds_pop(vm);
      ds_push(vm, a & b);
      break;
    }
    case Op::OR:
    {
      int32_t b = ds_pop(vm), a = ds_pop(vm);
      ds_push(vm, a | b);
      break;
    }
    case Op::XOR:
    {
      int32_t b = ds_pop(vm), a = ds_pop(vm);
      ds_push(vm, a ^ b);
      break;
    }
    case Op::INVERT: /* optional */
    {
      int32_t a = ds_pop(vm);
      ds_push(vm, ~a);
      break;
    }

    /* -------- Control flow -------- */
    case Op::JMP:
    {
      if (ip + 2 > ip_end)
        return static_cast<v4_err>(Err::TruncatedJump);
      int16_t off = read_i16_le(ip);
      ip += 2;
      const uint8_t *tgt = ip + off;
      if (tgt < bc || tgt > ip_end)
        return static_cast<v4_err>(Err::JumpOutOfRange);
      ip = tgt;
      break;
    }
    case Op::JZ:
    {
      if (ip + 2 > ip_end)
        return static_cast<v4_err>(Err::TruncatedJump);
      int16_t off = read_i16_le(ip);
      ip += 2;
      int32_t cond = ds_pop(vm);
      if (cond == 0)
      {
        const uint8_t *tgt = ip + off;
        if (tgt < bc || tgt > ip_end)
          return static_cast<v4_err>(Err::JumpOutOfRange);
        ip = tgt;
      }
      break;
    }
    case Op::JNZ:
    {
      if (ip + 2 > ip_end)
        return static_cast<v4_err>(Err::TruncatedJump);
      int16_t off = read_i16_le(ip);
      ip += 2;
      int32_t cond = ds_pop(vm);
      if (cond != 0)
      {
        const uint8_t *tgt = ip + off;
        if (tgt < bc || tgt > ip_end)
          return static_cast<v4_err>(Err::JumpOutOfRange);
        ip = tgt;
      }
      break;
    }

    /* -------- Memory (32-bit, little-endian) -------- */
    case Op::LOAD: /* ( addr -- x ) */
    {
      v4_u32 addr = (v4_u32)ds_pop(vm);
      v4_u32 val = 0;
      if (v4_err e = v4_mem_read32_core(vm, addr, &val))
        return e;
      ds_push(vm, (int32_t)val);
      break;
    }

    case Op::STORE: /* ( x addr -- ) */
    {
      v4_u32 addr = (v4_u32)ds_pop(vm); // top = addr
      v4_u32 val = (v4_u32)ds_pop(vm);  // next = x
      if (v4_err e = v4_mem_write32_core(vm, addr, val))
        return e;
      break;
    }

    /* -------- Return -------- */
    case Op::RET:
      return static_cast<v4_err>(Err::OK);

    default:
      return static_cast<v4_err>(Err::UnknownOp);
    }
  }

  // Reached end of bytecode without hitting RET.
  return static_cast<v4_err>(Err::FellOffEnd);
}

/* =========================== Public API entry ============================ */
/* Word-based execution (Tier-0 stub):
 *  For now, we don't have a defined Word layout in this file.
 *  This function is a stub that returns OK to keep linkage and API stable.
 *  Once Word{ bc,len,... } is solidified, make this call vm_exec_raw(...).   */

extern "C" v4_err vm_exec(Vm *vm, Word *entry)
{
  (void)vm;
  (void)entry;
  // TODO: adapt to Word-based calling convention and dispatch.
  return static_cast<v4_err>(Err::OK);
}
