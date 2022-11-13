/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>

#include <mcl/stdint.hpp>

#include "dynarmic/frontend/A64/a64_location_descriptor.h"

namespace Dynarmic::Backend::Arm64 {

struct A64JitState {
    std::array<u64, 31> reg{};
    u64 sp = 0;
    u64 pc = 0;

    u32 cpsr_nzcv = 0;

    u32 upper_location_descriptor;

    alignas(16) std::array<u64, 64> vec{};  // Extension registers.

    u32 exclusive_state = 0;

    u32 fpsr = 0;
    u32 fpcr = 0;

    IR::LocationDescriptor GetLocationDescriptor() const {
        return IR::LocationDescriptor{pc};
    }
};

}  // namespace Dynarmic::Backend::Arm64
