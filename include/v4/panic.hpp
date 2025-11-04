#pragma once
#include "v4/panic.h"
#include "v4/errors.hpp"
#include <cstdint>

namespace v4 {

/**
 * @brief VMパニック時の診断情報（C++ラッパー）
 *
 * C APIのV4PanicInfoをC++で扱いやすくするラッパークラス。
 */
struct PanicInfo {
    Err error;               /**< エラーコード（Err列挙型） */
    uint32_t pc;             /**< Program Counter */
    int32_t tos;             /**< Top of Stack（有効時のみ） */
    int32_t nos;             /**< Next on Stack（有効時のみ） */
    uint8_t ds_depth;        /**< Data Stack深さ */
    uint8_t rs_depth;        /**< Return Stack深さ */
    bool has_stack_data;     /**< スタックデータが有効か */

    /**
     * @brief C API構造体から変換
     */
    static PanicInfo from_c(const V4PanicInfo& c_info) {
        return PanicInfo{
            static_cast<Err>(c_info.error_code),
            c_info.pc,
            c_info.tos,
            c_info.nos,
            c_info.ds_depth,
            c_info.rs_depth,
            c_info.has_stack_data
        };
    }

    /**
     * @brief C API構造体へ変換
     */
    V4PanicInfo to_c() const {
        return V4PanicInfo{
            static_cast<int32_t>(error),
            pc,
            tos,
            nos,
            ds_depth,
            rs_depth,
            has_stack_data
        };
    }
};

}  // namespace v4
