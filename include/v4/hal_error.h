#ifndef V4_HAL_ERROR_H
#define V4_HAL_ERROR_H

/**
 * @file hal_error.h
 * @brief Error code definitions for V4 HAL
 *
 * All HAL functions return int (v4_err compatible).
 * 0 indicates success, negative values indicate errors.
 *
 * Error codes are defined using X-macro pattern in hal_errors.def.
 */

#ifdef __cplusplus
extern "C"
{
#endif

  // Define error codes using X-macro
#define HAL_ERR(name, val, desc) HAL_ERR_##name = val,
  enum
  {
#include "hal_errors.def"
  };
#undef HAL_ERR

  // Define HAL_OK as alias for HAL_ERR_OK
#define HAL_OK HAL_ERR_OK

  /**
   * @brief Get error message string
   *
   * @param err Error code
   * @return Human-readable error description
   */
  const char* hal_strerror(int err);

#ifdef __cplusplus
}
#endif

#endif  // V4_HAL_ERROR_H
