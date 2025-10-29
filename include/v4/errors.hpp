#pragma once

// Error code definition using the same pattern as opcodes.def
// Define ERR(name, val, msg) before including this file if you want to extract text or
// mapping.

#ifndef ERR
#define ERR(name, val, msg) name = val,
#endif

enum class Err : int
{
#include "v4/errors.def"
};

#undef ERR

// Macro to reduce code size for error returns
#define V4_ERR(name) static_cast<v4_err>(Err::name)

// Optional: helper to get a string message for each error
inline const char *err_str(Err e)
{
  switch (e)
  {
#define ERR(name, val, msg) \
  case Err::name:           \
    return msg;
#include "v4/errors.def"
#undef ERR
    default:
      return "unknown error";
  }
}
