/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>

#include <mcl/stdint.hpp>

namespace Dynarmic::Backend::Arm64 {

constexpr size_t SpillCount = 64;

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4324)  // Structure was padded due to alignment specifier
#endif

struct alignas(16) StackLayout {
    std::array<std::array<u64, 2>, SpillCount> spill;

    s64 cycles_to_run;

    u32 save_host_fpcr;

    bool check_bit;
};

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

static_assert(sizeof(StackLayout) % 16 == 0);

}  // namespace Dynarmic::Backend::Arm64
