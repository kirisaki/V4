#pragma once
#include <cstdint>

#include "v4/errors.hpp"
#include "v4/panic.h"

namespace v4
{

/**
 * @brief VM panic diagnostic information (C++ wrapper)
 *
 * C++ wrapper class for V4PanicInfo to make it easier to use in C++.
 */
struct PanicInfo
{
  Err error;           /**< Error code (Err enumeration) */
  uint32_t pc;         /**< Program Counter */
  int32_t tos;         /**< Top of Stack (when valid) */
  int32_t nos;         /**< Next on Stack (when valid) */
  uint8_t ds_depth;    /**< Data Stack depth */
  uint8_t rs_depth;    /**< Return Stack depth */
  bool has_stack_data; /**< Whether stack data is valid */

  /**
   * @brief Convert from C API structure
   */
  static PanicInfo from_c(const V4PanicInfo& c_info)
  {
    return PanicInfo{static_cast<Err>(c_info.error_code),
                     c_info.pc,
                     c_info.tos,
                     c_info.nos,
                     c_info.ds_depth,
                     c_info.rs_depth,
                     c_info.has_stack_data};
  }

  /**
   * @brief Convert to C API structure
   */
  V4PanicInfo to_c() const
  {
    return V4PanicInfo{
        static_cast<int32_t>(error), pc, tos, nos, ds_depth, rs_depth, has_stack_data};
  }
};

}  // namespace v4
