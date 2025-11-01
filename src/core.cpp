#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "v4/arena.h"
#include "v4/errors.hpp"
#include "v4/hal.h"
#include "v4/internal/memory.hpp"
#include "v4/internal/vm.h"
#include "v4/opcodes.hpp"
#include "v4/sys_ids.h"
#include "v4/vm_api.h"

/* ========================================================================= */
/* UART Handle Management                                                    */
/* ========================================================================= */

// Static storage for UART handles (indexed by port number)
// Maximum 4 UART ports (typical for most platforms)
#define MAX_UART_PORTS 4
static hal_handle_t uart_handles[MAX_UART_PORTS] = {nullptr, nullptr, nullptr, nullptr};

extern "C" int v4_vm_version(void)
{
  return 0;
}

extern "C" void vm_reset_dictionary(Vm* vm)
{
  if (!vm)
    return;

  // Free word names only if using malloc (not arena)
  if (!vm->arena)
  {
    for (int i = 0; i < vm->word_count; i++)
    {
      if (vm->words[i].name)
      {
        free(vm->words[i].name);
        vm->words[i].name = nullptr;
      }
    }
  }
  // If using arena, names are managed by arena owner

  vm->word_count = 0;
}

extern "C" void vm_reset_stacks(Vm* vm)
{
  if (!vm)
    return;

  // Reset data/return stacks to initial positions
  vm->sp = vm->DS;
  vm->rp = vm->RS;
  vm->fp = nullptr;  // No local frame initially
}

extern "C" void vm_reset(Vm* vm)
{
  if (!vm)
    return;

  // Reset dictionary and stacks
  vm_reset_dictionary(vm);
  vm_reset_stacks(vm);
}

/* ========================== Data stack helpers =========================== */

static inline v4_err ds_push(Vm* vm, v4_i32 v)
{
  if (vm->sp >= vm->DS + 256)
    return V4_ERR(StackOverflow);
  *vm->sp++ = v;
  return V4_ERR(OK);
}

static inline v4_err ds_pop(Vm* vm, v4_i32* out)
{
  if (vm->sp <= vm->DS)
    return V4_ERR(StackUnderflow);
  *out = *--vm->sp;
  return V4_ERR(OK);
}

static inline v4_err ds_peek(const Vm* vm, int i, v4_i32* out)
{
  if (vm->sp - 1 - i < vm->DS)
    return V4_ERR(StackUnderflow);
  *out = *(vm->sp - 1 - i);
  return V4_ERR(OK);
}

/* ========================== Return stack helpers ========================= */

static inline v4_err rs_push(Vm* vm, v4_i32 v)
{
  if (vm->rp >= vm->RS + 64)
    return V4_ERR(StackOverflow);
  *vm->rp++ = v;
  return V4_ERR(OK);
}

static inline v4_err rs_pop(Vm* vm, v4_i32* out)
{
  if (vm->rp <= vm->RS)
    return V4_ERR(StackUnderflow);
  *out = *--vm->rp;
  return V4_ERR(OK);
}

static inline v4_err rs_peek(const Vm* vm, int i, v4_i32* out)
{
  if (vm->rp - 1 - i < vm->RS)
    return V4_ERR(StackUnderflow);
  *out = *(vm->rp - 1 - i);
  return V4_ERR(OK);
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
          return V4_ERR(TruncatedLiteral);
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
          return V4_ERR(DivByZero);
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
          return V4_ERR(DivByZero);
        if (v4_err e = ds_push(vm, a % b))
          return e;
        break;
      }

      case v4::Op::DIVU:
      {
        v4_u32 a, b;
        v4_i32 a_i32, b_i32;
        if (v4_err e = ds_pop(vm, &b_i32))
          return e;
        if (v4_err e = ds_pop(vm, &a_i32))
          return e;
        b = (v4_u32)b_i32;
        a = (v4_u32)a_i32;
        if (b == 0)
          return V4_ERR(DivByZero);
        if (v4_err e = ds_push(vm, (v4_i32)(a / b)))
          return e;
        break;
      }

      case v4::Op::MODU:
      {
        v4_u32 a, b;
        v4_i32 a_i32, b_i32;
        if (v4_err e = ds_pop(vm, &b_i32))
          return e;
        if (v4_err e = ds_pop(vm, &a_i32))
          return e;
        b = (v4_u32)b_i32;
        a = (v4_u32)a_i32;
        if (b == 0)
          return V4_ERR(DivByZero);
        if (v4_err e = ds_push(vm, (v4_i32)(a % b)))
          return e;
        break;
      }

      case v4::Op::INC:
      {
        v4_i32 a;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a + 1))
          return e;
        break;
      }

      case v4::Op::DEC:
      {
        v4_i32 a;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_push(vm, a - 1))
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

      case v4::Op::LTU:
      {
        v4_u32 a, b;
        v4_i32 a_i32, b_i32;
        if (v4_err e = ds_pop(vm, &b_i32))
          return e;
        if (v4_err e = ds_pop(vm, &a_i32))
          return e;
        b = (v4_u32)b_i32;
        a = (v4_u32)a_i32;
        if (v4_err e = ds_push(vm, a < b ? V4_TRUE : V4_FALSE))
          return e;
        break;
      }

      case v4::Op::LEU:
      {
        v4_u32 a, b;
        v4_i32 a_i32, b_i32;
        if (v4_err e = ds_pop(vm, &b_i32))
          return e;
        if (v4_err e = ds_pop(vm, &a_i32))
          return e;
        b = (v4_u32)b_i32;
        a = (v4_u32)a_i32;
        if (v4_err e = ds_push(vm, a <= b ? V4_TRUE : V4_FALSE))
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

      case v4::Op::SHL:
      {
        v4_i32 shift, val;
        if (v4_err e = ds_pop(vm, &shift))
          return e;
        if (v4_err e = ds_pop(vm, &val))
          return e;
        if (v4_err e = ds_push(vm, val << (shift & 0x1F)))
          return e;
        break;
      }

      case v4::Op::SHR:
      {
        v4_i32 shift, val_i32;
        v4_u32 val;
        if (v4_err e = ds_pop(vm, &shift))
          return e;
        if (v4_err e = ds_pop(vm, &val_i32))
          return e;
        val = (v4_u32)val_i32;
        if (v4_err e = ds_push(vm, (v4_i32)(val >> (shift & 0x1F))))
          return e;
        break;
      }

      case v4::Op::SAR:
      {
        v4_i32 shift, val;
        if (v4_err e = ds_pop(vm, &shift))
          return e;
        if (v4_err e = ds_pop(vm, &val))
          return e;
        if (v4_err e = ds_push(vm, val >> (shift & 0x1F)))
          return e;
        break;
      }

      /* -------- Control flow -------- */
      case v4::Op::JMP:
      {
        if (ip + 2 > ip_end)
          return V4_ERR(TruncatedJump);
        int16_t off = read_i16_le(ip);
        ip += 2;
        const v4_u8* tgt = ip + off;
        if (tgt < bc || tgt > ip_end)
          return V4_ERR(JumpOutOfRange);
        ip = tgt;
        break;
      }

      case v4::Op::JZ:
      {
        if (ip + 2 > ip_end)
          return V4_ERR(TruncatedJump);
        int16_t off = read_i16_le(ip);
        ip += 2;
        v4_i32 cond;
        if (v4_err e = ds_pop(vm, &cond))
          return e;
        if (cond == 0)
        {
          const v4_u8* tgt = ip + off;
          if (tgt < bc || tgt > ip_end)
            return V4_ERR(JumpOutOfRange);
          ip = tgt;
        }
        break;
      }

      case v4::Op::JNZ:
      {
        if (ip + 2 > ip_end)
          return V4_ERR(TruncatedJump);
        int16_t off = read_i16_le(ip);
        ip += 2;
        v4_i32 cond;
        if (v4_err e = ds_pop(vm, &cond))
          return e;
        if (cond != 0)
        {
          const v4_u8* tgt = ip + off;
          if (tgt < bc || tgt > ip_end)
            return V4_ERR(JumpOutOfRange);
          ip = tgt;
        }
        break;
      }

      case v4::Op::SELECT:
      {
        v4_i32 a, b, flag;
        if (v4_err e = ds_pop(vm, &a))
          return e;
        if (v4_err e = ds_pop(vm, &b))
          return e;
        if (v4_err e = ds_pop(vm, &flag))
          return e;
        if (v4_err e = ds_push(vm, flag ? a : b))
          return e;
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

      case v4::Op::LOAD8U:
      {
        v4_i32 addr_i32;
        if (v4_err e = ds_pop(vm, &addr_i32))
          return e;
        v4_u32 addr = (v4_u32)addr_i32;
        v4_u32 val = 0;
        if (v4_err e = v4_mem_read8_core(vm, addr, &val))
          return e;
        if (v4_err e = ds_push(vm, (v4_i32)val))
          return e;
        break;
      }

      case v4::Op::LOAD16U:
      {
        v4_i32 addr_i32;
        if (v4_err e = ds_pop(vm, &addr_i32))
          return e;
        v4_u32 addr = (v4_u32)addr_i32;
        v4_u32 val = 0;
        if (v4_err e = v4_mem_read16_core(vm, addr, &val))
          return e;
        if (v4_err e = ds_push(vm, (v4_i32)val))
          return e;
        break;
      }

      case v4::Op::STORE8:
      {
        v4_i32 addr_i32, val_i32;
        if (v4_err e = ds_pop(vm, &addr_i32))
          return e;
        if (v4_err e = ds_pop(vm, &val_i32))
          return e;
        v4_u32 addr = (v4_u32)addr_i32;
        v4_u32 val = (v4_u32)val_i32;
        if (v4_err e = v4_mem_write8_core(vm, addr, val))
          return e;
        break;
      }

      case v4::Op::STORE16:
      {
        v4_i32 addr_i32, val_i32;
        if (v4_err e = ds_pop(vm, &addr_i32))
          return e;
        if (v4_err e = ds_pop(vm, &val_i32))
          return e;
        v4_u32 addr = (v4_u32)addr_i32;
        v4_u32 val = (v4_u32)val_i32;
        if (v4_err e = v4_mem_write16_core(vm, addr, val))
          return e;
        break;
      }

      case v4::Op::LOAD8S:
      {
        v4_i32 addr_i32;
        if (v4_err e = ds_pop(vm, &addr_i32))
          return e;
        v4_u32 addr = (v4_u32)addr_i32;
        v4_u32 val = 0;
        if (v4_err e = v4_mem_read8_core(vm, addr, &val))
          return e;
        // Sign-extend 8-bit to 32-bit
        int8_t val_i8 = (int8_t)(val & 0xFF);
        if (v4_err e = ds_push(vm, (v4_i32)val_i8))
          return e;
        break;
      }

      case v4::Op::LOAD16S:
      {
        v4_i32 addr_i32;
        if (v4_err e = ds_pop(vm, &addr_i32))
          return e;
        v4_u32 addr = (v4_u32)addr_i32;
        v4_u32 val = 0;
        if (v4_err e = v4_mem_read16_core(vm, addr, &val))
          return e;
        // Sign-extend 16-bit to 32-bit
        int16_t val_i16 = (int16_t)(val & 0xFFFF);
        if (v4_err e = ds_push(vm, (v4_i32)val_i16))
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

      /* -------- Compact literals -------- */
      case v4::Op::LIT0:
      {
        if (v4_err e = ds_push(vm, 0))
          return e;
        break;
      }

      case v4::Op::LIT1:
      {
        if (v4_err e = ds_push(vm, 1))
          return e;
        break;
      }

      case v4::Op::LITN1:
      {
        if (v4_err e = ds_push(vm, -1))
          return e;
        break;
      }

      case v4::Op::LIT_U8:
      {
        if (ip >= ip_end)
          return V4_ERR(TruncatedLiteral);
        v4_u32 val = (v4_u32)*ip++;
        if (v4_err e = ds_push(vm, (v4_i32)val))
          return e;
        break;
      }

      case v4::Op::LIT_I8:
      {
        if (ip >= ip_end)
          return V4_ERR(TruncatedLiteral);
        int8_t val = (int8_t)*ip++;
        if (v4_err e = ds_push(vm, (v4_i32)val))
          return e;
        break;
      }

      case v4::Op::LIT_I16:
      {
        if (ip + 2 > ip_end)
          return V4_ERR(TruncatedLiteral);
        int16_t val = read_i16_le(ip);
        ip += 2;
        if (v4_err e = ds_push(vm, (v4_i32)val))
          return e;
        break;
      }

      /* -------- Local variables -------- */
      case v4::Op::LGET:
      {
        if (ip >= ip_end)
          return V4_ERR(TruncatedLiteral);
        uint8_t idx = *ip++;
        if (!vm->fp)
          return V4_ERR(InvalidArg);  // No local frame
        if (vm->fp + idx >= vm->rp)
          return V4_ERR(StackUnderflow);  // Out of bounds
        if (v4_err e = ds_push(vm, vm->fp[idx]))
          return e;
        break;
      }

      case v4::Op::LSET:
      {
        if (ip >= ip_end)
          return V4_ERR(TruncatedLiteral);
        uint8_t idx = *ip++;
        if (!vm->fp)
          return V4_ERR(InvalidArg);  // No local frame
        if (vm->fp + idx >= vm->rp)
          return V4_ERR(StackUnderflow);  // Out of bounds
        v4_i32 val;
        if (v4_err e = ds_pop(vm, &val))
          return e;
        vm->fp[idx] = val;
        break;
      }

      case v4::Op::LTEE:
      {
        if (ip >= ip_end)
          return V4_ERR(TruncatedLiteral);
        uint8_t idx = *ip++;
        if (!vm->fp)
          return V4_ERR(InvalidArg);  // No local frame
        if (vm->fp + idx >= vm->rp)
          return V4_ERR(StackUnderflow);  // Out of bounds
        v4_i32 val;
        if (v4_err e = ds_peek(vm, 0, &val))
          return e;
        vm->fp[idx] = val;
        break;
      }

      case v4::Op::LGET0:
      {
        if (!vm->fp)
          return V4_ERR(InvalidArg);
        if (vm->fp >= vm->rp)
          return V4_ERR(StackUnderflow);
        if (v4_err e = ds_push(vm, vm->fp[0]))
          return e;
        break;
      }

      case v4::Op::LGET1:
      {
        if (!vm->fp)
          return V4_ERR(InvalidArg);
        if (vm->fp + 1 >= vm->rp)
          return V4_ERR(StackUnderflow);
        if (v4_err e = ds_push(vm, vm->fp[1]))
          return e;
        break;
      }

      case v4::Op::LSET0:
      {
        if (!vm->fp)
          return V4_ERR(InvalidArg);
        if (vm->fp >= vm->rp)
          return V4_ERR(StackUnderflow);
        v4_i32 val;
        if (v4_err e = ds_pop(vm, &val))
          return e;
        vm->fp[0] = val;
        break;
      }

      case v4::Op::LSET1:
      {
        if (!vm->fp)
          return V4_ERR(InvalidArg);
        if (vm->fp + 1 >= vm->rp)
          return V4_ERR(StackUnderflow);
        v4_i32 val;
        if (v4_err e = ds_pop(vm, &val))
          return e;
        vm->fp[1] = val;
        break;
      }

      case v4::Op::LINC:
      {
        if (ip >= ip_end)
          return V4_ERR(TruncatedLiteral);
        uint8_t idx = *ip++;
        if (!vm->fp)
          return V4_ERR(InvalidArg);
        if (vm->fp + idx >= vm->rp)
          return V4_ERR(StackUnderflow);
        vm->fp[idx]++;
        break;
      }

      case v4::Op::LDEC:
      {
        if (ip >= ip_end)
          return V4_ERR(TruncatedLiteral);
        uint8_t idx = *ip++;
        if (!vm->fp)
          return V4_ERR(InvalidArg);
        if (vm->fp + idx >= vm->rp)
          return V4_ERR(StackUnderflow);
        vm->fp[idx]--;
        break;
      }

      /* -------- Call / Return -------- */
      case v4::Op::CALL:
      {
        if (ip + 2 > ip_end)
          return V4_ERR(TruncatedJump);
        uint16_t word_idx = (uint16_t)ip[0] | ((uint16_t)ip[1] << 8);
        ip += 2;

        if (word_idx >= (uint16_t)vm->word_count)
          return V4_ERR(InvalidWordIdx);

        Word* word = &vm->words[word_idx];
        if (!word->code || word->code_len <= 0)
          return V4_ERR(InvalidArg);

        // Save current frame pointer (for nested calls)
        v4_i32* old_fp = vm->fp;

        // Set new frame pointer to current return stack position
        // This allows local variables to be accessed relative to the frame base
        vm->fp = vm->rp;

        // Execute the called word
        v4_err e = vm_exec_raw(vm, word->code, word->code_len);

        // Restore frame pointer after call returns
        vm->fp = old_fp;

        if (e)
          return e;
        break;
      }

      case v4::Op::SYS:
      {
        // Read SYS ID
        if (ip >= ip_end)
          return V4_ERR(TruncatedLiteral);
        uint8_t sys_id = *ip++;

        v4_err err = V4_ERR(OK);

        switch (sys_id)
        {
          /* GPIO operations */
          case V4_SYS_GPIO_INIT:  // (pin mode -- err)
          {
            v4_i32 mode, pin;
            if ((err = ds_pop(vm, &mode)))
              return err;
            if ((err = ds_pop(vm, &pin)))
              return err;

            int hal_err = hal_gpio_mode(pin, static_cast<hal_gpio_mode_t>(mode));
            if ((err = ds_push(vm, hal_err)))
              return err;
            break;
          }

          case V4_SYS_GPIO_WRITE:  // (pin value -- err)
          {
            v4_i32 value, pin;
            if ((err = ds_pop(vm, &value)))
              return err;
            if ((err = ds_pop(vm, &pin)))
              return err;

            int hal_err = hal_gpio_write(pin, static_cast<hal_gpio_value_t>(value));
            if ((err = ds_push(vm, hal_err)))
              return err;
            break;
          }

          case V4_SYS_GPIO_READ:  // (pin -- value err)
          {
            v4_i32 pin;
            if ((err = ds_pop(vm, &pin)))
              return err;

            hal_gpio_value_t value;
            int hal_err = hal_gpio_read(pin, &value);
            if ((err = ds_push(vm, static_cast<v4_i32>(value))))
              return err;
            if ((err = ds_push(vm, hal_err)))
              return err;
            break;
          }

          /* UART operations */
          case V4_SYS_UART_INIT:  // (port baudrate -- err)
          {
            v4_i32 baudrate, port;
            if ((err = ds_pop(vm, &baudrate)))
              return err;
            if ((err = ds_pop(vm, &port)))
              return err;

            // Validate port number
            if (port < 0 || port >= MAX_UART_PORTS)
            {
              if ((err = ds_push(vm, HAL_ERR_PARAM)))
                return err;
              break;
            }

            // Close existing handle if port already open
            if (uart_handles[port] != nullptr)
            {
              hal_uart_close(uart_handles[port]);
              uart_handles[port] = nullptr;
            }

            // Create UART config with default settings (C++17 compatible)
            hal_uart_config_t config = {
                baudrate,  // baudrate
                8,         // data_bits (standard)
                1,         // stop_bits (standard)
                0          // parity (no parity)
            };

            // Open UART and store handle
            uart_handles[port] = hal_uart_open(port, &config);
            int hal_err = (uart_handles[port] != nullptr) ? HAL_OK : HAL_ERR_IO;

            if ((err = ds_push(vm, hal_err)))
              return err;
            break;
          }

          case V4_SYS_UART_PUTC:  // (port char -- err)
          {
            v4_i32 ch, port;
            if ((err = ds_pop(vm, &ch)))
              return err;
            if ((err = ds_pop(vm, &port)))
              return err;

            // Validate port number and check if UART is open
            if (port < 0 || port >= MAX_UART_PORTS || uart_handles[port] == nullptr)
            {
              if ((err = ds_push(vm, HAL_ERR_NODEV)))
                return err;
              break;
            }

            // Write single character to UART
            uint8_t byte = static_cast<uint8_t>(ch);
            int result = hal_uart_write(uart_handles[port], &byte, 1);
            int hal_err = (result == 1) ? HAL_OK : ((result < 0) ? result : HAL_ERR_IO);

            if ((err = ds_push(vm, hal_err)))
              return err;
            break;
          }

          case V4_SYS_UART_GETC:  // (port -- char err)
          {
            v4_i32 port;
            if ((err = ds_pop(vm, &port)))
              return err;

            // Validate port number and check if UART is open
            if (port < 0 || port >= MAX_UART_PORTS || uart_handles[port] == nullptr)
            {
              if ((err = ds_push(vm, 0)))  // Push dummy char value
                return err;
              if ((err = ds_push(vm, HAL_ERR_NODEV)))
                return err;
              break;
            }

            // Read single character from UART (non-blocking)
            uint8_t byte = 0;
            int result = hal_uart_read(uart_handles[port], &byte, 1);
            int hal_err = (result >= 0) ? HAL_OK : result;

            if ((err = ds_push(vm, static_cast<v4_i32>(byte))))
              return err;
            if ((err = ds_push(vm, hal_err)))
              return err;
            break;
          }

          /* Timer operations */
          case V4_SYS_MILLIS:  // ( -- ms)
          {
            uint32_t ms = hal_millis();
            if ((err = ds_push(vm, static_cast<v4_i32>(ms))))
              return err;
            break;
          }

          case V4_SYS_MICROS:  // ( -- us_lo us_hi)
          {
            uint64_t us = hal_micros();
            uint32_t us_lo = static_cast<uint32_t>(us & 0xFFFFFFFF);
            uint32_t us_hi = static_cast<uint32_t>(us >> 32);
            if ((err = ds_push(vm, static_cast<v4_i32>(us_lo))))
              return err;
            if ((err = ds_push(vm, static_cast<v4_i32>(us_hi))))
              return err;
            break;
          }

          case V4_SYS_DELAY_MS:  // (ms -- )
          {
            v4_i32 ms;
            if ((err = ds_pop(vm, &ms)))
              return err;

            hal_delay_ms(static_cast<uint32_t>(ms));
            break;
          }

          case V4_SYS_DELAY_US:  // (us -- )
          {
            v4_i32 us;
            if ((err = ds_pop(vm, &us)))
              return err;

            hal_delay_us(static_cast<uint32_t>(us));
            break;
          }

          /* Console I/O operations */
          case V4_SYS_EMIT:  // (c -- )
          {
            v4_i32 c;
            if ((err = ds_pop(vm, &c)))
              return err;

            // Write single character to console
            uint8_t byte = static_cast<uint8_t>(c);
            int result = hal_console_write(&byte, 1);
            int hal_err = (result == 1) ? HAL_OK : ((result < 0) ? result : HAL_ERR_IO);

            if ((err = ds_push(vm, hal_err)))
              return err;
            break;
          }

          case V4_SYS_KEY:  // ( -- c)
          {
            // Read single character from console (blocking)
            uint8_t byte = 0;
            int result = hal_console_read(&byte, 1);
            int hal_err = (result >= 0) ? HAL_OK : result;

            if ((err = ds_push(vm, static_cast<v4_i32>(byte))))
              return err;
            if ((err = ds_push(vm, hal_err)))
              return err;
            break;
          }

          /* System operations */
          case V4_SYS_SYSTEM_RESET:  // ( -- )
          {
            // NOTE: System reset not yet available in new HAL API
            // TODO: Add system reset support to V4-hal library
            // For now, return "not supported" error
            if ((err = ds_push(vm, HAL_ERR_NOTSUP)))
              return err;
            break;
          }

          case V4_SYS_SYSTEM_INFO:  // ( -- addr len)
          {
            // NOTE: System info not yet available in new HAL API
            // TODO: Add system info support to V4-hal library
            // For now, return empty string and "not supported" error
            if ((err = ds_push(vm, 0)))  // addr = NULL
              return err;
            if ((err = ds_push(vm, 0)))  // len = 0
              return err;
            if ((err = ds_push(vm, HAL_ERR_NOTSUP)))
              return err;
            break;
          }

          default:
            return V4_ERR(UnknownOp);
        }
        break;
      }

      case v4::Op::RET:
        return V4_ERR(OK);

      default:
        return V4_ERR(UnknownOp);
    }
  }

  return V4_ERR(FellOffEnd);
}

/* ======================= Word management API ============================= */

extern "C" int vm_register_word(Vm* vm, const char* name, const uint8_t* code,
                                int code_len)
{
  if (!vm || !code || code_len <= 0)
    return V4_ERR(InvalidArg);

  if (vm->word_count >= Vm::V4_MAX_WORDS)
    return V4_ERR(DictionaryFull);

  int idx = vm->word_count;

  // Copy name if provided (NULL is allowed for anonymous words)
  if (name)
  {
    size_t len = strlen(name) + 1;  // +1 for null terminator

    // Use arena if available, otherwise malloc
    if (vm->arena)
    {
      char* name_copy = static_cast<char*>(v4_arena_alloc(vm->arena, len, 1));
      if (!name_copy)
        return V4_ERR(InvalidArg);  // Arena allocation failed
      memcpy(name_copy, name, len);
      vm->words[idx].name = name_copy;
    }
    else
    {
      vm->words[idx].name = strdup(name);
      if (!vm->words[idx].name)
        return V4_ERR(InvalidArg);  // strdup failed (out of memory)
    }
  }
  else
  {
    vm->words[idx].name = nullptr;
  }

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

extern "C" int vm_find_word(Vm* vm, const char* name)
{
  if (!vm || !name)
    return -1;

  for (int i = 0; i < vm->word_count; i++)
  {
    if (vm->words[i].name && strcmp(vm->words[i].name, name) == 0)
    {
      return i;
    }
  }

  return -1;  // Not found
}

/* =========================== Public API entry ============================ */

extern "C" v4_err vm_exec(Vm* vm, Word* entry)
{
  if (!vm || !entry || !entry->code)
    return V4_ERR(InvalidArg);

  return vm_exec_raw(vm, entry->code, entry->code_len);
}

/* ======================= Stack inspection API ============================ */

extern "C" int vm_ds_depth_public(struct Vm* vm)
{
  if (!vm)
    return 0;
  return (int)(vm->sp - vm->DS);
}

extern "C" v4_i32 vm_ds_peek_public(struct Vm* vm, int index_from_top)
{
  if (!vm)
    return 0;

  int depth = (int)(vm->sp - vm->DS);
  if (index_from_top < 0 || index_from_top >= depth)
    return 0;  // Out of range

  return vm->sp[-1 - index_from_top];
}

extern "C" int vm_ds_copy_to_array(struct Vm* vm, v4_i32* out_array, int max_count)
{
  if (!vm || !out_array || max_count <= 0)
    return 0;

  int depth = static_cast<int>(vm->sp - vm->DS);
  int copy_count = (depth < max_count) ? depth : max_count;

  // Copy stack from bottom to top
  std::memcpy(out_array, vm->DS, copy_count * sizeof(v4_i32));

  return copy_count;
}
/* ===================== Stack manipulation API ============================ */

extern "C" v4_err vm_ds_push(struct Vm* vm, v4_i32 value)
{
  if (!vm)
    return V4_ERR(InvalidArg);
  return ds_push(vm, value);
}

extern "C" v4_err vm_ds_pop(struct Vm* vm, v4_i32* out_value)
{
  if (!vm)
    return V4_ERR(InvalidArg);

  v4_i32 val;
  v4_err err = ds_pop(vm, &val);
  if (err == 0 && out_value)
    *out_value = val;
  return err;
}

extern "C" void vm_ds_clear(struct Vm* vm)
{
  if (!vm)
    return;
  vm->sp = vm->DS;
}

/* ==================== Return stack inspection API ======================== */

extern "C" int vm_rs_depth_public(struct Vm* vm)
{
  if (!vm)
    return 0;
  return static_cast<int>(vm->rp - vm->RS);
}

extern "C" int vm_rs_copy_to_array(struct Vm* vm, v4_i32* out_array, int max_count)
{
  if (!vm || !out_array || max_count <= 0)
    return 0;

  int depth = static_cast<int>(vm->rp - vm->RS);
  int copy_count = (depth < max_count) ? depth : max_count;

  // Copy stack from bottom to top
  std::memcpy(out_array, vm->RS, copy_count * sizeof(v4_i32));

  return copy_count;
}

/* ==================== Stack snapshot API ================================= */

extern "C" struct VmStackSnapshot* vm_ds_snapshot(struct Vm* vm)
{
  if (!vm)
    return nullptr;

  int depth = (int)(vm->sp - vm->DS);
  if (depth == 0)
  {
    // Empty stack - return snapshot with NULL data
    VmStackSnapshot* snap = (VmStackSnapshot*)malloc(sizeof(VmStackSnapshot));
    if (!snap)
      return nullptr;
    snap->data = nullptr;
    snap->depth = 0;
    return snap;
  }

  VmStackSnapshot* snap = (VmStackSnapshot*)malloc(sizeof(VmStackSnapshot));
  if (!snap)
    return nullptr;

  snap->data = (v4_i32*)malloc(depth * sizeof(v4_i32));
  if (!snap->data)
  {
    free(snap);
    return nullptr;
  }

  // Copy stack contents (bottom to top)
  memcpy(snap->data, vm->DS, depth * sizeof(v4_i32));
  snap->depth = depth;

  return snap;
}

extern "C" v4_err vm_ds_restore(struct Vm* vm, const struct VmStackSnapshot* snapshot)
{
  if (!vm || !snapshot)
    return V4_ERR(InvalidArg);

  // Check if restored stack would fit
  if (snapshot->depth > 256)
    return V4_ERR(StackOverflow);

  // Clear current stack
  vm->sp = vm->DS;

  // Restore from snapshot (bottom to top)
  if (snapshot->depth > 0 && snapshot->data)
  {
    memcpy(vm->DS, snapshot->data, snapshot->depth * sizeof(v4_i32));
    vm->sp = vm->DS + snapshot->depth;
  }

  return V4_ERR(OK);
}

extern "C" void vm_ds_snapshot_free(struct VmStackSnapshot* snapshot)
{
  if (!snapshot)
    return;

  if (snapshot->data)
    free(snapshot->data);
  free(snapshot);
}
