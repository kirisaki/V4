#pragma once
#include <cstdint>

/** Tier-0 opcode set (single byte). Immediates are little-endian. */
enum class Op : std::uint8_t
{
#define OP(name, val, _) name = val,
#include <v4/opcodes.def>
#undef OP
};
