/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <bit>
#include <tuple>
#include <unordered_map>

#include <xbyak/xbyak.h>

#include "dynarmic/common/common_types.h"

namespace Dynarmic::Backend::X64 {

class BlockOfCode;

/// ConstantPool allocates a block of memory from BlockOfCode.
/// It places constants into this block of memory, returning the address
/// of the memory location where the constant is placed. If the constant
/// already exists, its memory location is reused.
class ConstantPool final {
public:
    ConstantPool(BlockOfCode& code, size_t size);

    Xbyak::Address GetConstant(const Xbyak::AddressFrame& frame, u64 lower, u64 upper = 0);

private:
    static constexpr size_t align_size = 16;  // bytes

    struct ConstantHash {
        std::size_t operator()(const std::tuple<u64, u64>& constant) const {
            return std::get<0>(constant) ^ std::rotl<u64>(std::get<1>(constant), 1);
        }
    };

    std::unordered_map<std::tuple<u64, u64>, void*, ConstantHash> constant_info;

    BlockOfCode& code;
    size_t pool_size;
    u8* pool_begin;
    u8* current_pool_ptr;
};

}  // namespace Dynarmic::Backend::X64
