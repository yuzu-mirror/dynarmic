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
    RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8, R9, R10, R11, R12, R13, R14,
    CF, PF, AF, ZF, SF, OF,
    FirstSpill,
};

enum class HostLocState {
    Idle, Def, Use, Scratch
};

inline bool HostLocIsRegister(HostLoc reg) {
    return reg >= HostLoc::RAX && reg <= HostLoc::R14;
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

const std::initializer_list<HostLoc> hostloc_any_register = {
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
    HostLoc::R14
};

class RegAlloc final {
public:
    RegAlloc(Gen::XEmitter* code) : code(code) {}

    /// Late-def
    Gen::X64Reg DefRegister(IR::Value* def_value, std::initializer_list<HostLoc> desired_locations = hostloc_any_register);
    /// Early-use, Late-def
    Gen::X64Reg UseDefRegister(IR::Value* use_value, IR::Value* def_value, std::initializer_list<HostLoc> desired_locations = hostloc_any_register);
    /// Early-use
    Gen::X64Reg UseRegister(IR::Value* use_value, std::initializer_list<HostLoc> desired_locations = hostloc_any_register);
    /// Early-use, Destroyed
    Gen::X64Reg UseScratchRegister(IR::Value* use_value, std::initializer_list<HostLoc> desired_locations = hostloc_any_register);
    /// Early-def, Late-use, single-use
    Gen::X64Reg ScratchRegister(std::initializer_list<HostLoc> desired_locations = hostloc_any_register);

    /// Late-def for result register, Early-use for all arguments, Each value is placed into registers according to host ABI.
    void HostCall(IR::Value* result_def = nullptr, IR::Value* arg0_use = nullptr, IR::Value* arg1_use = nullptr, IR::Value* arg2_use = nullptr, IR::Value* arg3_use = nullptr);

    // TODO: Values in host flags

    void DecrementRemainingUses(IR::Value* value);

    void EndOfAllocScope();

    void AssertNoMoreUses();

    void Reset();

private:
    HostLoc SelectARegister(std::initializer_list<HostLoc> desired_locations) const;
    std::vector<HostLoc> ValueLocations(IR::Value* value) const;
    bool IsRegisterOccupied(HostLoc loc) const;
    bool IsRegisterAllocated(HostLoc loc) const;

    void SpillRegister(HostLoc loc);
    HostLoc FindFreeSpill() const;

    Gen::XEmitter* code = nullptr;

    using mapping_map_t = std::map<HostLoc, IR::Value*>;
    mapping_map_t hostloc_to_value;
    std::map<HostLoc, HostLocState> hostloc_state;
    std::map<IR::Value*, size_t> remaining_uses;
};

} // namespace BackendX64
} // namespace Dynarmic
