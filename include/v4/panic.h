#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VMパニック時の診断情報
 *
 * VMがエラー状態に陥った際の診断情報を保持する構造体。
 * vm_panic()関数によって収集され、診断出力に使用される。
 */
typedef struct V4PanicInfo {
    int32_t error_code;      /**< エラーコード（Err列挙値） */
    uint32_t pc;             /**< Program Counter */
    int32_t tos;             /**< Top of Stack（有効時のみ） */
    int32_t nos;             /**< Next on Stack（有効時のみ） */
    uint8_t ds_depth;        /**< Data Stack深さ */
    uint8_t rs_depth;        /**< Return Stack深さ */
    bool has_stack_data;     /**< スタックデータが有効か */
} V4PanicInfo;

#ifdef __cplusplus
}
#endif
