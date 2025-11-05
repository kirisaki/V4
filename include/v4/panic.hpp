/**
 * @file panic.hpp
 * @brief V4 VM panic handler C++ wrapper
 *
 * SPDX-License-Identifier: MIT OR Apache-2.0
 */

#pragma once

#include "v4/panic.h"

namespace v4
{

/**
 * @brief C++ wrapper for V4PanicInfo
 */
using PanicInfo = V4PanicInfo;

/**
 * @brief C++ wrapper for V4PanicHandler
 */
using PanicHandler = V4PanicHandler;

/**
 * @brief Set panic handler (C++ wrapper)
 *
 * @param vm         VM instance
 * @param handler    Panic handler callback
 * @param user_data  User data passed to handler
 */
inline void set_panic_handler(Vm *vm, PanicHandler handler, void *user_data = nullptr)
{
  vm_set_panic_handler(vm, handler, user_data);
}

}  // namespace v4
