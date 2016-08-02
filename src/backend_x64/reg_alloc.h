/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <map>

#include "backend_x64/jitstate.h"
#include "common/common_types.h"
#include "common/x64/emitter.h"
#include "frontend/ir/ir.h"

namespace Dynarmic {
namespace BackendX64 {

enum class HostLoc {
    // Ordering of the registers is intentional. See also: HostLocToX64.
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14,
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
    CF, PF, AF, ZF, SF, OF,
    FirstSpill,
};

constexpr size_t HostLocCount = static_cast<size_t>(HostLoc::FirstSpill) + SpillCount;

enum class HostLocState {
    Idle, Def, Use, Scratch
};

inline bool HostLocIsGPR(HostLoc reg) {
    return reg >= HostLoc::RAX && reg <= HostLoc::R14;
}

inline bool HostLocIsXMM(HostLoc reg) {
    return reg >= HostLoc::XMM0 && reg <= HostLoc::XMM15;
}

inline bool HostLocIsRegister(HostLoc reg) {
    return HostLocIsGPR(reg) || HostLocIsXMM(reg);
}

inline bool HostLocIsFlag(HostLoc reg) {
    return reg >= HostLoc::CF && reg <= HostLoc::OF;
}

inline HostLoc HostLocSpill(size_t i) {
    ASSERT_MSG(i < SpillCount, "Invalid spill");
    return static_cast<HostLoc>(static_cast<int>(HostLoc::FirstSpill) + i);
}

inline bool HostLocIsSpill(HostLoc reg) {
    return reg >= HostLoc::FirstSpill && reg <= HostLocSpill(SpillCount - 1);
}

using HostLocList = std::initializer_list<HostLoc>;

const HostLocList any_gpr = {
    HostLoc::RAX,
    HostLoc::RBX,
    HostLoc::RCX,
    HostLoc::RDX,
    HostLoc::RSI,
    HostLoc::RDI,
    HostLoc::RBP,
    HostLoc::RSP,
    HostLoc::R8,
    HostLoc::R9,
    HostLoc::R10,
    HostLoc::R11,
    HostLoc::R12,
    HostLoc::R13,
    HostLoc::R14,
};

const HostLocList any_xmm = {
    HostLoc::XMM0,
    HostLoc::XMM1,
    HostLoc::XMM2,
    HostLoc::XMM3,
    HostLoc::XMM4,
    HostLoc::XMM5,
    HostLoc::XMM6,
    HostLoc::XMM7,
    HostLoc::XMM8,
    HostLoc::XMM9,
    HostLoc::XMM10,
    HostLoc::XMM11,
    HostLoc::XMM12,
    HostLoc::XMM13,
    HostLoc::XMM14,
    HostLoc::XMM15,
};

class RegAlloc final {
public:
    RegAlloc(Gen::XEmitter* code) : code(code) {}

    /// Late-def
    Gen::X64Reg DefRegister(IR::Inst* def_inst, HostLocList desired_locations);
    /// Early-use, Late-def
    Gen::X64Reg UseDefRegister(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations);
    Gen::X64Reg UseDefRegister(IR::Inst* use_inst, IR::Inst* def_inst, HostLocList desired_locations);
    /// Early-use
    Gen::X64Reg UseRegister(IR::Value use_value, HostLocList desired_locations);
    Gen::X64Reg UseRegister(IR::Inst* use_inst, HostLocList desired_locations);
    /// Early-use, Destroyed
    Gen::X64Reg UseScratchRegister(IR::Value use_value, HostLocList desired_locations);
    Gen::X64Reg UseScratchRegister(IR::Inst* use_inst, HostLocList desired_locations);
    /// Early-def, Late-use, single-use
    Gen::X64Reg ScratchRegister(HostLocList desired_locations);
    Gen::X64Reg LoadImmediateIntoRegister(IR::Value imm, Gen::X64Reg reg);

    /// Late-def for result register, Early-use for all arguments, Each value is placed into registers according to host ABI.
    void HostCall(IR::Inst* result_def = nullptr, IR::Value arg0_use = {}, IR::Value arg1_use = {}, IR::Value arg2_use = {}, IR::Value arg3_use = {});

    // TODO: Values in host flags

    void DecrementRemainingUses(IR::Inst* value);

    void EndOfAllocScope();

    void AssertNoMoreUses();

    void Reset();

private:
    HostLoc SelectARegister(HostLocList desired_locations) const;
    std::vector<HostLoc> ValueLocations(IR::Inst* value) const;
    bool IsRegisterOccupied(HostLoc loc) const;
    bool IsRegisterAllocated(HostLoc loc) const;

    void EmitMove(HostLoc to, HostLoc from);
    void EmitExchange(HostLoc a, HostLoc b);
    void SpillRegister(HostLoc loc);
    HostLoc FindFreeSpill() const;

    Gen::XEmitter* code = nullptr;

    struct HostLocInfo {
        HostLocInfo() = default;
        HostLocInfo(IR::Inst* value, HostLocState state) : value(value), state(state) {}
        IR::Inst* value = nullptr;
        HostLocState state = HostLocState::Idle;
        IR::Type GetType() const {
            return value ? value->GetType() : IR::Type::Void;
        }
    };
    std::array<HostLocInfo, HostLocCount> hostloc_info;
    HostLocInfo& LocInfo(HostLoc loc) {
        return hostloc_info[static_cast<size_t>(loc)];
    }
    HostLocInfo GetLocInfo(HostLoc loc) const {
        return hostloc_info[static_cast<size_t>(loc)];
    }
};

} // namespace BackendX64
} // namespace Dynarmic
