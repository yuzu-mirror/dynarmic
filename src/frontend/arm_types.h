/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <cassert>
#include <tuple>
#include <type_traits>

#include "common/bit_util.h"
#include "common/common_types.h"

namespace Dynarmic {
namespace Arm {

enum class Cond {
    EQ, NE, CS, CC, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE, AL, NV
};

inline const char* CondToString(Cond cond, bool explicit_al = false) {
    constexpr std::array<const char*, 15> cond_strs = {
        "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "al"
    };
    return (!explicit_al && cond == Cond::AL) ? "" : cond_strs.at(static_cast<size_t>(cond));
}

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

inline const char* RegToString(Reg reg) {
    constexpr std::array<const char*, 16> reg_strs = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
    };
    return reg_strs.at(static_cast<size_t>(reg));
}

enum class ExtReg {
    S0, S1, S2, S3, S4, S5, S6, S7,
    S8, S9, S10, S11, S12, S13, S14, S15,
    S16, S17, S18, S19, S20, S21, S22, S23,
    S24, S25, S26, S27, S28, S29, S30, S31,
    D0, D1, D2, D3, D4, D5, D6, D7,
    D8, D9, D10, D11, D12, D13, D14, D15,
    D16, D17, D18, D19, D20, D21, D22, D23,
    D24, D25, D26, D27, D28, D29, D30, D31,
};

using Imm3 = u32;
using Imm4 = u32;
using Imm5 = u32;
using Imm7 = u32;
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
    static constexpr u32 FPSCR_MASK = 0x3F79F9F;

    LocationDescriptor(u32 arm_pc, bool tflag, bool eflag, u32 fpscr)
            : arm_pc(arm_pc), tflag(tflag), eflag(eflag), fpscr(fpscr & FPSCR_MASK) {}

    u32 PC() const { return arm_pc; }
    bool TFlag() const { return tflag; }
    bool EFlag() const { return eflag; }
    u32 FPSCR() const { return fpscr; }
    bool FPSCR_FTZ() const { return Common::Bit<24>(fpscr); }
    bool FPSCR_DN() const { return Common::Bit<25>(fpscr); }
    u32 FPSCR_Len() const { return Common::Bits<16, 18>(fpscr) + 1; }
    u32 FPSCR_Stride() const { return Common::Bits<20, 21>(fpscr) + 1; }

    bool operator == (const LocationDescriptor& o) const {
        return std::tie(arm_pc, tflag, eflag, fpscr) == std::tie(o.arm_pc, o.tflag, o.eflag, o.fpscr);
    }

    LocationDescriptor SetPC(u32 new_arm_pc) const {
        return LocationDescriptor(new_arm_pc, tflag, eflag, fpscr);
    }

    LocationDescriptor AdvancePC(s32 amount) const {
        return LocationDescriptor(arm_pc + amount, tflag, eflag, fpscr);
    }

    LocationDescriptor SetTFlag(bool new_tflag) const {
        return LocationDescriptor(arm_pc, new_tflag, eflag, fpscr);
    }

    LocationDescriptor SetEFlag(bool new_eflag) const {
        return LocationDescriptor(arm_pc, tflag, new_eflag, fpscr);
    }

    LocationDescriptor SetFPSCR(u32 new_fpscr) const {
        return LocationDescriptor(arm_pc, tflag, eflag, new_fpscr & FPSCR_MASK);
    }

private:
    u32 arm_pc;
    bool tflag; ///< Thumb / ARM
    bool eflag; ///< Big / Little Endian
    u32 fpscr;  ///< Floating point status control register
};

struct LocationDescriptorHash {
    size_t operator()(const LocationDescriptor& x) const {
        return std::hash<u64>()(static_cast<u64>(x.PC())
                                ^ static_cast<u64>(x.TFlag())
                                ^ (static_cast<u64>(x.EFlag()) << 1)
                                ^ (static_cast<u64>(x.FPSCR()) << 32));
    }
};

} // namespace Arm
} // namespace Dynarmic
