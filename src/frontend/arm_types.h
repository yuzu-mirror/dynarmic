/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <cassert>
#include <tuple>
#include <type_traits>

#include "common/common_types.h"

namespace Dynarmic {
namespace Arm {

enum class Cond {
    EQ, NE, CS, CC, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE, AL, NV
};

enum class Reg {
    R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15,
    SP = R13,
    LR = R14,
    PC = R15,
    INVALID_REG = 99
};

inline Reg operator+(Reg reg, int number) {
    assert(reg != Reg::INVALID_REG);

    int new_reg = static_cast<int>(reg) + number;
    assert(new_reg >= 0 && new_reg <= 15);

    return static_cast<Reg>(new_reg);
}

using Imm4 = u32;
using Imm5 = u32;
using Imm8 = u32;
using Imm11 = u32;
using Imm12 = u32;
using Imm24 = u32;
using RegList = u16;

enum class ShiftType {
    LSL,
    LSR,
    ASR,
    ROR ///< RRX falls under this too
};

enum class SignExtendRotation {
    ROR_0,  ///< ROR #0 or omitted
    ROR_8,  ///< ROR #8
    ROR_16, ///< ROR #16
    ROR_24  ///< ROR #24
};

struct LocationDescriptor {
    LocationDescriptor(u32 arm_pc, bool TFlag, bool EFlag, Cond cond = Cond::AL)
            : arm_pc(arm_pc), TFlag(TFlag), EFlag(EFlag), cond(cond) {}

    u32 arm_pc;
    bool TFlag; ///< Thumb / ARM
    bool EFlag; ///< Big / Little Endian
    Cond cond;

    bool operator == (const LocationDescriptor& o) const {
        return std::tie(arm_pc, TFlag, EFlag, cond) == std::tie(o.arm_pc, o.TFlag, o.EFlag, o.cond);
    }
};

} // namespace Arm
} // namespace Dynarmic
