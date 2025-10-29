#pragma once
#include <cstdint>

namespace v4
{

/** Tier-0 opcode set (single byte). Immediates are little-endian. */
enum class Op : std::uint8_t
{
#define OP(name, val, _) name = val,
#include <v4/opcodes.def>
#undef OP
};

// -----------------------------------------------------------------------------
// Immediate kind classification for front-end table-driven dispatch
// -----------------------------------------------------------------------------
enum class PrimKind : uint8_t
{
  NoImm,  // e.g., DUP, ADD, RET
  Imm8,   // SYS id8
  Imm16,  // LIT_I16 imm16 (signed 16-bit immediate)
  Imm32,  // LIT imm32
  Rel16,  // JMP/JZ/JNZ off16 (signed, byte offset, next-PC based)
  Idx16,  // CALL idx16 (word index)
};

// -----------------------------------------------------------------------------
// Primitive entry definition
// -----------------------------------------------------------------------------
struct PrimitiveEntry
{
  const char* name;
  uint8_t opcode;
  PrimKind kind;
};

// -----------------------------------------------------------------------------
// Helper macro to convert immediate kind token to PrimKind enum
// -----------------------------------------------------------------------------
#define PRIM_KIND_NO_IMM PrimKind::NoImm
#define PRIM_KIND_IMM8 PrimKind::Imm8
#define PRIM_KIND_IMM16 PrimKind::Imm16
#define PRIM_KIND_IMM32 PrimKind::Imm32
#define PRIM_KIND_REL16 PrimKind::Rel16
#define PRIM_KIND_IDX16 PrimKind::Idx16

// -----------------------------------------------------------------------------
// Tier-0 primitive table (auto-generated from opcodes.def)
// -----------------------------------------------------------------------------
static constexpr PrimitiveEntry kPrimitiveTable[] = {
#define OP(name, val, kind) {#name, val, PRIM_KIND_##kind},
#include <v4/opcodes.def>
#undef OP
};

// -----------------------------------------------------------------------------
// Optional SYS alias constants
// -----------------------------------------------------------------------------
enum : uint8_t
{
  SYS_EMIT = 1,
  SYS_KEY = 2,
  SYS_MILLIS = 3,
};

}  // namespace v4