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

extern "C" void vm_reset(Vm *vm)
{
  // Reset data/return stacks to initial positions.
  vm->sp = vm->DS;
  vm->rp = vm->RS;
}

/* ========================== Data stack helpers =========================== */

static inline v4_i32 ds_push(Vm *vm, int32_t v)
{
  if (vm->sp > vm->DS + 256)
    return static_cast<v4_err>(Err::StackOverflow);
  *vm->sp++ = v;
  return 0;
}
static inline v4_i32 ds_pop(Vm *vm)
{
  if (vm->sp < vm->DS)
    return static_cast<v4_err>(Err::StackUnderflow);
  return *--vm->sp;
}
static inline v4_i32 ds_peek(const Vm *vm, int i = 0)
{
  if (vm->sp - 1 - i > vm->DS)
    return static_cast<v4_err>(Err::StackUnderflow);
  return *(vm->sp - 1 - i);
}
static inline v4_i32 ds_poke(Vm *vm, int i, int32_t v)
{
  if (vm->sp - 1 - i < vm->DS)
    return static_cast<v4_err>(Err::StackUnderflow);
  *(vm->sp - 1 - i) = v;
  return 0;
}

/* ===================== Little-endian readers/writers ===================== */

static inline v4_i32 read_i32_le(const v4_u8 *p)
{
  return (v4_i32)((v4_u32)p[0] | ((uint32_t)p[1] << 8) | ((v4_u32)p[2] << 16) |
                  ((v4_i32)p[3] << 24));
}
static inline int16_t read_i16_le(const v4_u8 *p)
{
  return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* =================== Internal raw bytecode interpreter =================== */
/* This keeps your current loop intact so existing tests keep running.
 * Public API vm_exec(...) will call into a Word-based path later.          */

extern "C" v4_err vm_exec_raw(Vm *vm, const v4_u8 *bc, int len)
{
  assert(vm && bc && len > 0);
  const v4_u8 *ip = bc;
  const v4_u8 *ip_end = bc + len;

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
        v4_i32 k = read_i32_le(ip);
        ip += 4;
        ds_push(vm, k);
        break;
      }

      /* ---- Stack manipulation ---- */
      case Op::DUP:
      {
        v4_i32 a = ds_peek(vm, 0);
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
        v4_i32 a = ds_pop(vm);
        v4_i32 b = ds_pop(vm);
        ds_push(vm, a);
        ds_push(vm, b);
        break;
      }
      case Op::OVER:
      {
        v4_i32 v = ds_peek(vm, 1);
        ds_push(vm, v);
        break;
      }

      /* -------- Arithmetic -------- */
      case Op::ADD:
      {
        v4_i32 a = ds_pop(vm);
        v4_i32 b = ds_pop(vm);
        ds_push(vm, b + a);
        break;
      }
      case Op::SUB:
      {
        v4_i32 a = ds_pop(vm);
        v4_i32 b = ds_pop(vm);
        ds_push(vm, b - a);
        break;
      }
      case Op::MUL:
      {
        v4_i32 b = ds_pop(vm);
        v4_i32 a = ds_pop(vm);
        ds_push(vm, (v4_i32)(a * b));
        break;
      }
      case Op::DIV:
      {
        v4_i32 b = ds_pop(vm);
        v4_i32 a = ds_pop(vm);
        if (b == 0)
          return static_cast<v4_err>(Err::DivByZero);
        ds_push(vm, a / b);
        break;
      }
      case Op::MOD:
      {
        v4_i32 b = ds_pop(vm);
        v4_i32 a = ds_pop(vm);
        if (b == 0)
          return static_cast<v4_err>(Err::DivByZero);
        ds_push(vm, a % b);
        break;
      }

      /* -------- Comparison (Forth truth: -1 = true, 0 = false) -------- */
      case Op::EQ:
      {
        v4_i32 b = ds_pop(vm), a = ds_pop(vm);
        ds_push(vm, a == b ? V4_TRUE : V4_FALSE);
        break;
      }
      case Op::NE:
      {
        v4_i32 b = ds_pop(vm), a = ds_pop(vm);
        ds_push(vm, a != b ? V4_TRUE : V4_FALSE);
        break;
      }
      case Op::LT:
      {
        v4_i32 b = ds_pop(vm), a = ds_pop(vm);
        ds_push(vm, a < b ? V4_TRUE : V4_FALSE);
        break;
      }
      case Op::LE:
      {
        v4_i32 b = ds_pop(vm), a = ds_pop(vm);
        ds_push(vm, a <= b ? V4_TRUE : V4_FALSE);
        break;
      }
      case Op::GT:
      {
        v4_i32 b = ds_pop(vm), a = ds_pop(vm);
        ds_push(vm, a > b ? V4_TRUE : V4_FALSE);
        break;
      }
      case Op::GE:
      {
        v4_i32 b = ds_pop(vm), a = ds_pop(vm);
        ds_push(vm, a >= b ? V4_TRUE : V4_FALSE);
        break;
      }

      /* -------- Bitwise -------- */
      case Op::AND:
      {
        v4_i32 b = ds_pop(vm), a = ds_pop(vm);
        ds_push(vm, a & b);
        break;
      }
      case Op::OR:
      {
        v4_i32 b = ds_pop(vm), a = ds_pop(vm);
        ds_push(vm, a | b);
        break;
      }
      case Op::XOR:
      {
        v4_i32 b = ds_pop(vm), a = ds_pop(vm);
        ds_push(vm, a ^ b);
        break;
      }
      case Op::INVERT: /* optional */
      {
        v4_i32 a = ds_pop(vm);
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
        const v4_u8 *tgt = ip + off;
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
        v4_i32 cond = ds_pop(vm);
        if (cond == 0)
        {
          const v4_u8 *tgt = ip + off;
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
        v4_i32 cond = ds_pop(vm);
        if (cond != 0)
        {
          const v4_u8 *tgt = ip + off;
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
        ds_push(vm, (v4_i32)val);
        break;
      }

      case Op::STORE: /* ( x addr -- ) */
      {
        v4_u32 addr = (v4_u32)ds_pop(vm);  // top = addr
        v4_u32 val = (v4_u32)ds_pop(vm);   // next = x
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
