/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>

#include "common/common_types.h"

namespace Dynarmic {
namespace BackendX64 {

constexpr size_t SpillCount = 32;

struct JitState {
    u32 Cpsr = 0;
    std::array<u32, 16> Reg{}; // Current register file.
    // TODO: Mode-specific register sets unimplemented.

    alignas(u64) std::array<u32, 64> ExtReg{}; // Extension registers.

    std::array<u64, SpillCount> Spill{}; // Spill.

    // For internal use (See: BlockOfCode::RunCode)
    u32 guest_MXCSR = 0x00001f80;
    u32 save_host_MXCSR = 0;
    u64 save_host_RSP = 0;
    s64 cycles_remaining = 0;

    u32 FPSCR_IDC = 0;
    u32 FPSCR_UFC = 0;
    u32 guest_FPSCR_flags = 0;
    u32 old_FPSCR = 0;
    u32 Fpscr() const;
    void SetFpscr(u32 FPSCR);
};

using CodePtr = const u8*;

} // namespace BackendX64
} // namespace Dynarmic
