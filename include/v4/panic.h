#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief VM panic diagnostic information
   *
   * Structure holding diagnostic information when VM enters an error state.
   * Collected by vm_panic() function and used for diagnostic output.
   */
  typedef struct V4PanicInfo
  {
    int32_t error_code;  /**< Error code (Err enumeration value) */
    uint32_t pc;         /**< Program Counter */
    int32_t tos;         /**< Top of Stack (when valid) */
    int32_t nos;         /**< Next on Stack (when valid) */
    uint8_t ds_depth;    /**< Data Stack depth */
    uint8_t rs_depth;    /**< Return Stack depth */
    bool has_stack_data; /**< Whether stack data is valid */
  } V4PanicInfo;

#ifdef __cplusplus
}
#endif
