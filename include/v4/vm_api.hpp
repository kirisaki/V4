/**
 * @file vm_api.hpp
 * @brief V4 VM C++ API wrapper
 *
 * SPDX-License-Identifier: MIT OR Apache-2.0
 */

#pragma once

#include "v4/vm_api.h"

namespace v4
{

/**
 * @brief C++ wrapper for Word accessors
 */
class Word
{
public:
  /**
   * @brief Get word name
   * @param word  Opaque Word pointer
   * @return Word name, or nullptr if invalid
   */
  static const char *get_name(const struct ::Word *word)
  {
    return vm_word_get_name(word);
  }

  /**
   * @brief Get word bytecode
   * @param word  Opaque Word pointer
   * @return Bytecode pointer, or nullptr if invalid
   */
  static const v4_u8 *get_code(const struct ::Word *word)
  {
    return vm_word_get_code(word);
  }

  /**
   * @brief Get word bytecode length
   * @param word  Opaque Word pointer
   * @return Bytecode length in bytes
   */
  static v4_u16 get_code_len(const struct ::Word *word)
  {
    return vm_word_get_code_len(word);
  }
};

}  // namespace v4
