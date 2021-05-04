/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>

#include "common/common_types.h"

namespace Dynarmic::Backend::X64 {

constexpr size_t SpillCount = 64;

struct alignas(16) StackLayout {
    std::array<std::array<u64, 2>, SpillCount> spill;

    u32 save_host_MXCSR;

    bool check_bit;
};

static_assert(sizeof(StackLayout) % 16 == 0);

} // namespace Dynarmic::Backend::X64
