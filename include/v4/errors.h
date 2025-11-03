#pragma once

/**
 * @file errors.h
 * @brief V4 error codes for C
 *
 * C-compatible error code definitions generated from errors.def
 */

#ifdef __cplusplus
extern "C"
{
#endif

  /* Generate error code constants from errors.def */
#define ERR(name, val, msg) static const int V4_ERR_##name = val;
#include "v4/errors.def"
#undef ERR

#ifdef __cplusplus
}
#endif
