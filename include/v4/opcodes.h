#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /** @file
   *  @brief Tier-0 opcode set for C.
   *
   *  Single-byte opcodes. Immediates are little-endian.
   *  Keep numeric values stable once published.
   */

  typedef enum v4_op_t
  {
#define OP(name, val, _) V4_OP_##name = val,
#include <v4/opcodes.def>
#undef OP
  } v4_op_t;

#ifdef __cplusplus
}  // extern "C"
#endif
