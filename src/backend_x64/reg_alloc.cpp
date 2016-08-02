/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <algorithm>
#include <map>

#include "backend_x64/reg_alloc.h"
#include "common/assert.h"
#include "common/x64/emitter.h"

namespace Dynarmic {
namespace BackendX64 {

static Gen::X64Reg HostLocToX64(HostLoc loc) {
    DEBUG_ASSERT(HostLocIsRegister(loc));
    // HostLoc is ordered such that the numbers line up.
    return static_cast<Gen::X64Reg>(loc);
}

static Gen::OpArg SpillToOpArg(HostLoc loc) {
    static_assert(std::is_same<decltype(JitState{}.Spill[0]), u64&>::value, "Spill must be u64");
    DEBUG_ASSERT(HostLocIsSpill(loc));

    size_t i = static_cast<size_t>(loc) - static_cast<size_t>(HostLoc::FirstSpill);
    return Gen::MDisp(Gen::R15, static_cast<int>(offsetof(JitState, Spill) + i * sizeof(u64)));
}

Gen::X64Reg RegAlloc::DefRegister(IR::Inst* def_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(ValueLocations(def_inst).empty(), "def_inst has already been defined");

    HostLoc location = SelectARegister(desired_locations);

    if (IsRegisterOccupied(location)) {
        SpillRegister(location);
    }

    // Update state
    LocInfo(location) = {def_inst, HostLocState::Def};
    return HostLocToX64(location);
}

Gen::X64Reg RegAlloc::UseDefRegister(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseDefRegister(use_value.GetInst(), def_inst, desired_locations);
    }

    return LoadImmediateIntoRegister(use_value, DefRegister(def_inst, desired_locations));
}

Gen::X64Reg RegAlloc::UseDefRegister(IR::Inst* use_inst, IR::Inst* def_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(ValueLocations(def_inst).empty(), "def_inst has already been defined");
    DEBUG_ASSERT_MSG(!ValueLocations(use_inst).empty(), "use_inst has not been defined");

    // TODO: Optimize the case when this is the last use_inst use.
    Gen::X64Reg use_reg = UseRegister(use_inst, any_gpr);
    Gen::X64Reg def_reg = DefRegister(def_inst, desired_locations);
    code->MOV(64, Gen::R(def_reg), Gen::R(use_reg));
    return def_reg;
}

Gen::X64Reg RegAlloc::UseRegister(IR::Value use_value, HostLocList desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseRegister(use_value.GetInst(), desired_locations);
    }

    return LoadImmediateIntoRegister(use_value, ScratchRegister(desired_locations));
}

Gen::X64Reg RegAlloc::UseRegister(IR::Inst* use_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(!ValueLocations(use_inst).empty(), "use_inst has not been defined");

    HostLoc current_location = ValueLocations(use_inst).front();
    auto iter = std::find(desired_locations.begin(), desired_locations.end(), current_location);
    if (iter != desired_locations.end()) {
        ASSERT(LocInfo(current_location).state == HostLocState::Idle ||
               LocInfo(current_location).state == HostLocState::Use);

        // Update state
        LocInfo(current_location).state = HostLocState::Use;
        DecrementRemainingUses(use_inst);

        return HostLocToX64(current_location);
    }

    HostLoc new_location = SelectARegister(desired_locations);

    if (HostLocIsSpill(current_location)) {
        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
        }

        EmitMove(new_location, current_location);

        LocInfo(new_location) = LocInfo(current_location);
        LocInfo(new_location).state = HostLocState::Use;
        LocInfo(current_location) = {};
        DecrementRemainingUses(use_inst);
    } else if (HostLocIsRegister(current_location)) {
        ASSERT(LocInfo(current_location).state == HostLocState::Idle);

        EmitExchange(new_location, current_location);

        std::swap(LocInfo(new_location), LocInfo(current_location));
        LocInfo(new_location).state = HostLocState::Use;
        DecrementRemainingUses(use_inst);
    } else {
        ASSERT_MSG(0, "Invalid current_location");
    }

    return HostLocToX64(new_location);
}

Gen::X64Reg RegAlloc::UseScratchRegister(IR::Value use_value, HostLocList desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseScratchRegister(use_value.GetInst(), desired_locations);
    }

    return LoadImmediateIntoRegister(use_value, ScratchRegister(desired_locations));
}

Gen::X64Reg RegAlloc::UseScratchRegister(IR::Inst* use_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(!ValueLocations(use_inst).empty(), "use_inst has not been defined");
    ASSERT_MSG(use_inst->use_count != 0, "use_inst ran out of uses. (Use-d an IR::Inst* too many times)");

    HostLoc current_location = ValueLocations(use_inst).front();
    HostLoc new_location = SelectARegister(desired_locations);

    if (HostLocIsSpill(current_location)) {
        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
        }

        EmitMove(new_location, current_location);

        LocInfo(new_location).state = HostLocState::Scratch;
        DecrementRemainingUses(use_inst);
    } else if (HostLocIsRegister(current_location)) {
        ASSERT(LocInfo(current_location).state == HostLocState::Idle);

        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
            if (current_location != new_location) {
                EmitMove(new_location, current_location);
            }
        } else {
            EmitMove(new_location, current_location);
        }

        LocInfo(new_location).state = HostLocState::Scratch;
        DecrementRemainingUses(use_inst);
    } else {
        ASSERT_MSG(0, "Invalid current_location");
    }

    return HostLocToX64(new_location);
}

Gen::X64Reg RegAlloc::ScratchRegister(HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));

    HostLoc location = SelectARegister(desired_locations);

    if (IsRegisterOccupied(location)) {
        SpillRegister(location);
    }

    // Update state
    LocInfo(location).state = HostLocState::Scratch;

    return HostLocToX64(location);
}

Gen::X64Reg RegAlloc::LoadImmediateIntoRegister(IR::Value imm, Gen::X64Reg reg) {
    ASSERT_MSG(imm.IsImmediate(), "imm is not an immediate");

    switch (imm.GetType()) {
        case IR::Type::U1:
            code->MOV(32, R(reg), Gen::Imm32(imm.GetU1()));
            break;
        case IR::Type::U8:
            code->MOV(32, R(reg), Gen::Imm32(imm.GetU8()));
            break;
        case IR::Type::U32:
            code->MOV(32, R(reg), Gen::Imm32(imm.GetU32()));
            break;
        default:
            ASSERT_MSG(false, "This should never happen.");
    }

    return reg;
}

void RegAlloc::HostCall(IR::Inst* result_def, IR::Value arg0_use, IR::Value arg1_use, IR::Value arg2_use, IR::Value arg3_use) {
    constexpr HostLoc AbiReturn = HostLoc::RAX;
#ifdef _WIN32
    constexpr std::array<HostLoc, 4> AbiArgs = { HostLoc::RCX, HostLoc::RDX, HostLoc::R8, HostLoc::R9 };
    /// Caller-saved registers other than AbiReturn or AbiArgs
    constexpr std::array<HostLoc, 2> OtherCallerSave = { HostLoc::R10, HostLoc::R11 };
#else
    constexpr std::array<HostLoc, 4> AbiArgs = { HostLoc::RDI, HostLoc::RSI, HostLoc::RDX, HostLoc::RCX };
    /// Caller-saved registers other than AbiReturn or AbiArgs
    constexpr std::array<HostLoc, 4> OtherCallerSave = { HostLoc::R8, HostLoc::R9, HostLoc::R10, HostLoc::R11 };
#endif

    const std::array<IR::Value*, 4> args = {&arg0_use, &arg1_use, &arg2_use, &arg3_use};

    // TODO: This works but almost certainly leads to suboptimal generated code.

    for (HostLoc caller_save : OtherCallerSave) {
        ScratchRegister({caller_save});
    }

    if (result_def) {
        DefRegister(result_def, {AbiReturn});
    } else {
        ScratchRegister({AbiReturn});
    }

    for (size_t i = 0; i < AbiArgs.size(); i++) {
        if (!args[i]->IsEmpty()) {
            UseScratchRegister(*args[i], {AbiArgs[i]});
        } else {
            ScratchRegister({AbiArgs[i]});
        }
    }

    ScratchRegister({HostLoc::RSP});
    code->MOV(64, Gen::R(Gen::RSP), Gen::MDisp(Gen::R15, offsetof(JitState, save_host_RSP)));
}

HostLoc RegAlloc::SelectARegister(HostLocList desired_locations) const {
    std::vector<HostLoc> candidates = desired_locations;

    // Find all locations that have not been allocated..
    auto allocated_locs = std::partition(candidates.begin(), candidates.end(), [this](auto loc){
        return !this->IsRegisterAllocated(loc);
    });
    candidates.erase(allocated_locs, candidates.end());
    ASSERT_MSG(!candidates.empty(), "All candidate registers have already been allocated");

    // Selects the best location out of the available locations.
    // TODO: Actually do LRU or something. Currently we just try to pick something without a value if possible.

    std::partition(candidates.begin(), candidates.end(), [this](auto loc){
        return !this->IsRegisterOccupied(loc);
    });

    return candidates.front();
}

std::vector<HostLoc> RegAlloc::ValueLocations(IR::Inst* value) const {
    std::vector<HostLoc> locations;

    for (size_t i = 0; i < HostLocCount; i++)
        if (hostloc_info[i].value == value)
            locations.emplace_back(static_cast<HostLoc>(i));

    return locations;
}

bool RegAlloc::IsRegisterOccupied(HostLoc loc) const {
    return GetLocInfo(loc).value != nullptr;
}

bool RegAlloc::IsRegisterAllocated(HostLoc loc) const {
    return GetLocInfo(loc).state != HostLocState::Idle;
}

void RegAlloc::SpillRegister(HostLoc loc) {
    ASSERT_MSG(HostLocIsRegister(loc), "Only registers can be spilled");
    ASSERT_MSG(LocInfo(loc).state == HostLocState::Idle, "Allocated registers cannot be spilled");
    ASSERT_MSG(IsRegisterOccupied(loc), "There is no need to spill unoccupied registers");
    ASSERT_MSG(!IsRegisterAllocated(loc), "Registers that have been allocated must not be spilt");

    HostLoc new_loc = FindFreeSpill();

    EmitMove(new_loc, loc);

    LocInfo(new_loc).value = LocInfo(loc).value;
    LocInfo(loc).value = nullptr;
}

HostLoc RegAlloc::FindFreeSpill() const {
    for (size_t i = 0; i < SpillCount; i++)
        if (!IsRegisterOccupied(HostLocSpill(i)))
            return HostLocSpill(i);

    ASSERT_MSG(0, "All spill locations are full");
}

void RegAlloc::EndOfAllocScope() {
    for (auto& iter : hostloc_info) {
        iter.state = HostLocState::Idle;
        if (iter.value && iter.value->use_count == 0)
            iter.value = nullptr;
    }
}

void RegAlloc::DecrementRemainingUses(IR::Inst* value) {
    ASSERT_MSG(value->use_count > 0, "value doesn't have any remaining uses");
    value->use_count--;
}

void RegAlloc::AssertNoMoreUses() {
    ASSERT(std::all_of(hostloc_info.begin(), hostloc_info.end(), [](const auto& i){ return !i.value; }));
}

void RegAlloc::Reset() {
    hostloc_info.fill({});
}

void RegAlloc::EmitMove(HostLoc to, HostLoc from) {
    const auto& from_info = LocInfo(from);

    if (HostLocIsXMM(to) && HostLocIsSpill(from)) {
        if (from_info.GetType() == IR::Type::F64) {
            code->MOVSD(HostLocToX64(to), SpillToOpArg(from));
        } else if (from_info.GetType() == IR::Type::F32) {
            code->MOVSS(HostLocToX64(to), SpillToOpArg(from));
        } else {
            ASSERT_MSG(false, "Tried to move a non-fp value into an XMM register");
        }
    } else if (HostLocIsSpill(to) && HostLocIsXMM(from)) {
        if (from_info.GetType() == IR::Type::F64) {
            code->MOVSD(SpillToOpArg(to), HostLocToX64(from));
        } else if (from_info.GetType() == IR::Type::F32) {
            code->MOVSS(SpillToOpArg(to), HostLocToX64(from));
        } else {
            ASSERT_MSG(false, "Tried to move a non-fp value into an XMM register");
        }
    } else if (HostLocIsXMM(to) && HostLocIsXMM(from)) {
        code->MOVAPS(HostLocToX64(to), Gen::R(HostLocToX64(from)));
    } else if (HostLocIsGPR(to) && HostLocIsSpill(from)) {
        code->MOV(64, Gen::R(HostLocToX64(to)), SpillToOpArg(from));
    } else if (HostLocIsSpill(to) && HostLocIsGPR(from)) {
        code->MOV(64, SpillToOpArg(to), Gen::R(HostLocToX64(from)));
    } else if (HostLocIsGPR(to) && HostLocIsGPR(from)){
        code->MOV(64, Gen::R(HostLocToX64(to)), Gen::R(HostLocToX64(from)));
    } else {
        ASSERT_MSG(false, "Invalid RegAlloc::EmitMove");
    }
}

void RegAlloc::EmitExchange(HostLoc a, HostLoc b) {
    if (HostLocIsGPR(a) && HostLocIsGPR(b)) {
        code->XCHG(64, Gen::R(HostLocToX64(a)), Gen::R(HostLocToX64(b)));
    } else if (HostLocIsXMM(a) && HostLocIsXMM(b)) {
        ASSERT_MSG(false, "Exchange is unnecessary for XMM registers");
    } else {
        ASSERT_MSG(false, "Invalid RegAlloc::EmitExchange");
    }
}

} // namespace BackendX64
} // namespace Dynarmic
