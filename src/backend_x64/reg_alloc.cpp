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
    DEBUG_ASSERT(HostLocIsSpill(loc));

    size_t i = static_cast<size_t>(loc) - static_cast<size_t>(HostLoc::FirstSpill);
    return Gen::MDisp(Gen::R15, static_cast<int>(offsetof(JitState, Spill) + i * sizeof(u32)));
}

Gen::X64Reg RegAlloc::DefRegister(IR::Inst* def_inst, std::initializer_list<HostLoc> desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(ValueLocations(def_inst).empty(), "def_inst has already been defined");

    HostLoc location = SelectARegister(desired_locations);

    if (IsRegisterOccupied(location)) {
        SpillRegister(location);
    }

    // Update state
    hostloc_state[static_cast<size_t>(location)] = HostLocState::Def;
    hostloc_to_inst[static_cast<size_t>(location)] = def_inst;

    return HostLocToX64(location);
}

Gen::X64Reg RegAlloc::UseDefRegister(IR::Value use_value, IR::Inst* def_inst, std::initializer_list<HostLoc> desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseDefRegister(use_value.GetInst(), def_inst, desired_locations);
    }

    return LoadImmediateIntoRegister(use_value, DefRegister(def_inst, desired_locations));
}

Gen::X64Reg RegAlloc::UseDefRegister(IR::Inst* use_inst, IR::Inst* def_inst, std::initializer_list<HostLoc> desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(ValueLocations(def_inst).empty(), "def_inst has already been defined");
    DEBUG_ASSERT_MSG(!ValueLocations(use_inst).empty(), "use_inst has not been defined");

    // TODO: Optimize the case when this is the last use_inst use.
    Gen::X64Reg use_reg = UseRegister(use_inst);
    Gen::X64Reg def_reg = DefRegister(def_inst, desired_locations);
    code->MOV(32, Gen::R(def_reg), Gen::R(use_reg));
    return def_reg;
}

Gen::X64Reg RegAlloc::UseRegister(IR::Value use_value, std::initializer_list<HostLoc> desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseRegister(use_value.GetInst(), desired_locations);
    }

    return LoadImmediateIntoRegister(use_value, ScratchRegister(desired_locations));
}

Gen::X64Reg RegAlloc::UseRegister(IR::Inst* use_inst, std::initializer_list<HostLoc> desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(!ValueLocations(use_inst).empty(), "use_inst has not been defined");

    HostLoc current_location = ValueLocations(use_inst).front();
    auto iter = std::find(desired_locations.begin(), desired_locations.end(), current_location);
    if (iter != desired_locations.end()) {
        ASSERT(hostloc_state[static_cast<size_t>(current_location)] == HostLocState::Idle || hostloc_state[static_cast<size_t>(current_location)] == HostLocState::Use);

        // Update state
        hostloc_state[static_cast<size_t>(current_location)] = HostLocState::Use;
        DecrementRemainingUses(use_inst);

        return HostLocToX64(current_location);
    }

    HostLoc new_location = SelectARegister(desired_locations);

    if (HostLocIsSpill(current_location)) {
        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
        }

        code->MOV(32, Gen::R(HostLocToX64(new_location)), SpillToOpArg(current_location));

        hostloc_state[static_cast<size_t>(new_location)] = HostLocState::Use;
        std::swap(hostloc_to_inst[static_cast<size_t>(new_location)], hostloc_to_inst[static_cast<size_t>(current_location)]);
        DecrementRemainingUses(use_inst);
    } else if (HostLocIsRegister(current_location)) {
        ASSERT(hostloc_state[static_cast<size_t>(current_location)] == HostLocState::Idle);

        code->XCHG(32, Gen::R(HostLocToX64(new_location)), Gen::R(HostLocToX64(current_location)));

        hostloc_state[static_cast<size_t>(new_location)] = HostLocState::Use;
        std::swap(hostloc_to_inst[static_cast<size_t>(new_location)], hostloc_to_inst[static_cast<size_t>(current_location)]);
        DecrementRemainingUses(use_inst);
    } else {
        ASSERT_MSG(0, "Invalid current_location");
    }

    return HostLocToX64(new_location);
}

Gen::X64Reg RegAlloc::UseScratchRegister(IR::Value use_value, std::initializer_list<HostLoc> desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseScratchRegister(use_value.GetInst(), desired_locations);
    }

    return LoadImmediateIntoRegister(use_value, ScratchRegister(desired_locations));
}

Gen::X64Reg RegAlloc::UseScratchRegister(IR::Inst* use_inst, std::initializer_list<HostLoc> desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(!ValueLocations(use_inst).empty(), "use_inst has not been defined");
    ASSERT_MSG(use_inst->use_count != 0, "use_inst ran out of uses. (Use-d an IR::Inst* too many times)");

    HostLoc current_location = ValueLocations(use_inst).front();
    HostLoc new_location = SelectARegister(desired_locations);

    if (HostLocIsSpill(current_location)) {
        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
        }

        code->MOV(32, Gen::R(HostLocToX64(new_location)), SpillToOpArg(current_location));

        hostloc_state[static_cast<size_t>(new_location)] = HostLocState::Scratch;
        DecrementRemainingUses(use_inst);
    } else if (HostLocIsRegister(current_location)) {
        ASSERT(hostloc_state[static_cast<size_t>(current_location)] == HostLocState::Idle);

        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
            if (current_location != new_location) {
                code->MOV(32, Gen::R(HostLocToX64(new_location)), Gen::R(HostLocToX64(current_location)));
            }
        } else {
            code->MOV(32, Gen::R(HostLocToX64(new_location)), Gen::R(HostLocToX64(current_location)));
        }

        hostloc_state[static_cast<size_t>(new_location)] = HostLocState::Scratch;
        DecrementRemainingUses(use_inst);
    } else {
        ASSERT_MSG(0, "Invalid current_location");
    }

    return HostLocToX64(new_location);
}


Gen::X64Reg RegAlloc::ScratchRegister(std::initializer_list<HostLoc> desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));

    HostLoc location = SelectARegister(desired_locations);

    if (IsRegisterOccupied(location)) {
        SpillRegister(location);
    }

    // Update state
    hostloc_state[static_cast<size_t>(location)] = HostLocState::Scratch;

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

HostLoc RegAlloc::SelectARegister(std::initializer_list<HostLoc> desired_locations) const {
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
        if (hostloc_to_inst[i] == value)
            locations.emplace_back(static_cast<HostLoc>(i));

    return locations;
}

bool RegAlloc::IsRegisterOccupied(HostLoc loc) const {
    return hostloc_to_inst.at(static_cast<size_t>(loc)) != nullptr;
}

bool RegAlloc::IsRegisterAllocated(HostLoc loc) const {
    return hostloc_state.at(static_cast<size_t>(loc)) != HostLocState::Idle;
}

void RegAlloc::SpillRegister(HostLoc loc) {
    ASSERT_MSG(HostLocIsRegister(loc), "Only registers can be spilled");
    ASSERT_MSG(hostloc_state[static_cast<size_t>(loc)] == HostLocState::Idle, "Allocated registers cannot be spilled");
    ASSERT_MSG(IsRegisterOccupied(loc), "There is no need to spill unoccupied registers");
    ASSERT_MSG(!IsRegisterAllocated(loc), "Registers that have been allocated must not be spilt");

    HostLoc new_loc = FindFreeSpill();

    code->MOV(32, SpillToOpArg(new_loc), Gen::R(HostLocToX64(loc)));

    hostloc_to_inst[static_cast<size_t>(new_loc)] = hostloc_to_inst[static_cast<size_t>(loc)];
    hostloc_to_inst[static_cast<size_t>(loc)] = nullptr;
}

HostLoc RegAlloc::FindFreeSpill() const {
    for (size_t i = 0; i < SpillCount; i++)
        if (!IsRegisterOccupied(HostLocSpill(i)))
            return HostLocSpill(i);

    ASSERT_MSG(0, "All spill locations are full");
}

void RegAlloc::EndOfAllocScope() {
    hostloc_state.fill(HostLocState::Idle);

    for (auto& iter : hostloc_to_inst)
        if (iter && iter->use_count == 0)
            iter = nullptr;
}

void RegAlloc::DecrementRemainingUses(IR::Inst* value) {
    ASSERT_MSG(value->use_count > 0, "value doesn't have any remaining uses");
    value->use_count--;
}

void RegAlloc::AssertNoMoreUses() {
    ASSERT(std::all_of(hostloc_to_inst.begin(), hostloc_to_inst.end(), [](const auto& inst){ return !inst; }));
}

void RegAlloc::Reset() {
    hostloc_to_inst.fill(nullptr);
    hostloc_state.fill(HostLocState::Idle);
}

} // namespace BackendX64
} // namespace Dynarmic
