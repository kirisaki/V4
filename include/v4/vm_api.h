#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /** @file
   *  @brief V4-VM Tier-0 public C API.
   *
   *  This header exposes a small, stable C ABI for embedding the VM in
   *  embedded targets and CLI tools. The internal implementation is C++17,
   *  but the API remains C-compatible.
   *
   *  @defgroup v4vm V4-VM Public API
   *  @{
   */

  /** @brief Opaque VM state for Tier-0 interpreter.
   *
   *  Layout is currently fixed (Tier-0), but treat it as opaque from user code.
   *  Future releases may add fields; binary size remains stable when using
   *  the provided API.
   *
   *  @note Data stack (DS) and return stack (RS) have fixed capacities in Tier-0.
   */
  typedef struct Vm
  {
    int32_t DS[256]; /**< Data stack storage (Top at sp-1). */
    int32_t RS[64];  /**< Return stack storage (Top at rp-1). */
    int32_t *sp;     /**< Next push position for DS. */
    int32_t *rp;     /**< Next push position for RS. */
  } Vm;

  /** @brief API/ABI version for compatibility checks.
   *  @return Always returns 0 for the initial Tier-0 release.
   *  @since 0
   */
  int v4_vm_version(void);

  /** @brief Reset VM stacks to an empty state.
   *  @param vm Pointer to a valid VM instance.
   *  @warning Does not clear memory; only resets pointers.
   *  @since 0
   */
  void vm_reset(Vm *vm);

  /** @brief Execute a bytecode buffer with the Tier-0 interpreter.
   *
   *  Bytecode is interpreted from @p bc to @p bc+len. Execution stops on
   *  `RET` or error.
   *
   *  @param vm  Pointer to an initialized VM.
   *  @param bc  Pointer to the bytecode buffer (little-endian immediates).
   *  @param len Size of the buffer in bytes.
   *  @retval  0 Success (returned via `RET`).
   *  @retval -1 Truncated immediate (e.g., `LIT` missing bytes).
   *  @retval -2 Truncated branch offset (`JMP/JZ/JNZ`).
   *  @retval -3 Out-of-range jump target.
   *  @retval -10 Reached end-of-buffer without `RET`.
   *  @retval -99 Unknown/unimplemented opcode.
   *  @since 0
   */
  int vm_exec(Vm *vm, const uint8_t *bc, int len);

  /** @} */ /* end of group v4vm */

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus
/// \copydoc vm_reset
inline void vm_reset(Vm &vm) { vm_reset(&vm); }

/// \copydoc vm_exec
inline int vm_exec(Vm &vm, const uint8_t *bc, int len) { return vm_exec(&vm, bc, len); }
#endif
