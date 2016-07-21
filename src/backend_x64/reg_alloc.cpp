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

// TODO: Just turn this into a function that indexes a std::array.
const static std::map<HostLoc, Gen::X64Reg> hostloc_to_x64 = {
    { HostLoc::RAX, Gen::RAX },
    { HostLoc::RBX, Gen::RBX },
    { HostLoc::RCX, Gen::RCX },
    { HostLoc::RDX, Gen::RDX },
    { HostLoc::RSI, Gen::RSI },
    { HostLoc::RDI, Gen::RDI },
    { HostLoc::RBP, Gen::RBP },
    { HostLoc::RSP, Gen::RSP },
    { HostLoc::R8,  Gen::R8  },
    { HostLoc::R9,  Gen::R9  },
    { HostLoc::R10, Gen::R10 },
    { HostLoc::R11, Gen::R11 },
    { HostLoc::R12, Gen::R12 },
    { HostLoc::R13, Gen::R13 },
    { HostLoc::R14, Gen::R14 },
};

static Gen::OpArg SpillToOpArg(HostLoc loc) {
    ASSERT(HostLocIsSpill(loc));

    size_t i = static_cast<size_t>(loc) - static_cast<size_t>(HostLoc::FirstSpill);
    return Gen::MDisp(Gen::R15, static_cast<int>(offsetof(JitState, Spill) + i * sizeof(u32)));
}

Gen::X64Reg RegAlloc::DefRegister(IR::Value* def_value, std::initializer_list<HostLoc> desired_locations) {
    ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    ASSERT_MSG(remaining_uses.find(def_value) == remaining_uses.end(), "def_value has already been defined");
    ASSERT_MSG(ValueLocations(def_value).empty(), "def_value has already been defined");

    HostLoc location = SelectARegister(desired_locations);

    if (IsRegisterOccupied(location)) {
        SpillRegister(location);
    }

    // Update state
    hostloc_state[location] = HostLocState::Def;
    hostloc_to_value[location] = def_value;
    remaining_uses[def_value] = def_value->NumUses();

    return hostloc_to_x64.at(location);
}

Gen::X64Reg RegAlloc::UseDefRegister(IR::Value* use_value, IR::Value* def_value, std::initializer_list<HostLoc> desired_locations) {
    ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    ASSERT_MSG(remaining_uses.find(def_value) == remaining_uses.end(), "def_value has already been defined");
    ASSERT_MSG(ValueLocations(def_value).empty(), "def_value has already been defined");
    ASSERT_MSG(remaining_uses.find(use_value) != remaining_uses.end(), "use_value has not been defined");
    ASSERT_MSG(!ValueLocations(use_value).empty(), "use_value has not been defined");

    // TODO: Optimize the case when this is the last use_value use.
    Gen::X64Reg use_reg = UseRegister(use_value);
    Gen::X64Reg def_reg = DefRegister(def_value, desired_locations);
    code->MOV(32, Gen::R(def_reg), Gen::R(use_reg));
    return def_reg;
}

Gen::X64Reg RegAlloc::UseRegister(IR::Value* use_value, std::initializer_list<HostLoc> desired_locations) {
    ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    ASSERT_MSG(remaining_uses.find(use_value) != remaining_uses.end(), "use_value has not been defined");
    ASSERT_MSG(!ValueLocations(use_value).empty(), "use_value has not been defined");
    ASSERT_MSG(remaining_uses[use_value] != 0, "use_value ran out of uses. (Use-d an IR::Value* too many times)");

    HostLoc current_location = ValueLocations(use_value).front();
    auto iter = std::find(desired_locations.begin(), desired_locations.end(), current_location);
    if (iter != desired_locations.end()) {
        ASSERT(hostloc_state[current_location] == HostLocState::Idle || hostloc_state[current_location] == HostLocState::Use);

        // Update state
        hostloc_state[current_location] = HostLocState::Use;
        remaining_uses[use_value]--;

        return hostloc_to_x64.at(current_location);
    }

    HostLoc new_location = SelectARegister(desired_locations);

    if (HostLocIsSpill(current_location)) {
        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
        }

        code->MOV(32, Gen::R(hostloc_to_x64.at(new_location)), SpillToOpArg(current_location));

        hostloc_state[new_location] = HostLocState::Use;
        std::swap(hostloc_to_value[new_location], hostloc_to_value[current_location]);
        remaining_uses[use_value]--;
    } else if (HostLocIsRegister(current_location)) {
        ASSERT(hostloc_state[current_location] == HostLocState::Idle);

        code->XCHG(32, Gen::R(hostloc_to_x64.at(new_location)), Gen::R(hostloc_to_x64.at(current_location)));

        hostloc_state[new_location] = HostLocState::Use;
        std::swap(hostloc_to_value[new_location], hostloc_to_value[current_location]);
        remaining_uses[use_value]--;
    } else {
        ASSERT_MSG(0, "Invalid current_location");
    }

    return hostloc_to_x64.at(new_location);
}

Gen::X64Reg RegAlloc::UseScratchRegister(IR::Value* use_value, std::initializer_list<HostLoc> desired_locations) {
    ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    ASSERT_MSG(remaining_uses.find(use_value) != remaining_uses.end(), "use_value has not been defined");
    ASSERT_MSG(!ValueLocations(use_value).empty(), "use_value has not been defined");
    ASSERT_MSG(remaining_uses[use_value] != 0, "use_value ran out of uses. (Use-d an IR::Value* too many times)");

    HostLoc current_location = ValueLocations(use_value).front();
    HostLoc new_location = SelectARegister(desired_locations);

    if (HostLocIsSpill(current_location)) {
        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
        }

        code->MOV(32, Gen::R(hostloc_to_x64.at(new_location)), SpillToOpArg(current_location));

        hostloc_state[new_location] = HostLocState::Scratch;
        remaining_uses[use_value]--;
    } else if (HostLocIsRegister(current_location)) {
        ASSERT(hostloc_state[current_location] == HostLocState::Idle);

        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
            if (current_location != new_location) {
                code->MOV(32, Gen::R(hostloc_to_x64.at(new_location)), Gen::R(hostloc_to_x64.at(current_location)));
            }
        } else {
            code->MOV(32, Gen::R(hostloc_to_x64.at(new_location)), Gen::R(hostloc_to_x64.at(current_location)));
        }

        hostloc_state[new_location] = HostLocState::Scratch;
        remaining_uses[use_value]--;
    } else {
        ASSERT_MSG(0, "Invalid current_location");
    }

    return hostloc_to_x64.at(new_location);
}


Gen::X64Reg RegAlloc::ScratchRegister(std::initializer_list<HostLoc> desired_locations) {
    ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));

    HostLoc location = SelectARegister(desired_locations);

    if (IsRegisterOccupied(location)) {
        SpillRegister(location);
    }

    // Update state
    hostloc_state[location] = HostLocState::Scratch;

    return hostloc_to_x64.at(location);
}

void RegAlloc::HostCall(IR::Value* result_def, IR::Value* arg0_use, IR::Value* arg1_use, IR::Value* arg2_use, IR::Value* arg3_use) {
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

    const std::array<IR::Value*, 4> args = {arg0_use, arg1_use, arg2_use, arg3_use};

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
        if (args[i]) {
            UseScratchRegister(args[i], {AbiArgs[i]});
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

std::vector<HostLoc> RegAlloc::ValueLocations(IR::Value* value) const {
    std::vector<HostLoc> locations;

    for (const auto& iter : hostloc_to_value)
        if (iter.second == value)
            locations.emplace_back(iter.first);

    return locations;
}

bool RegAlloc::IsRegisterOccupied(HostLoc loc) const {
    return hostloc_to_value.find(loc) != hostloc_to_value.end() && hostloc_to_value.at(loc) != nullptr;
}

bool RegAlloc::IsRegisterAllocated(HostLoc loc) const {
    return hostloc_state.find(loc) != hostloc_state.end() && hostloc_state.at(loc) != HostLocState::Idle;
}

void RegAlloc::SpillRegister(HostLoc loc) {
    ASSERT_MSG(HostLocIsRegister(loc), "Only registers can be spilled");
    ASSERT_MSG(hostloc_state[loc] == HostLocState::Idle, "Allocated registers cannot be spilled");
    ASSERT_MSG(IsRegisterOccupied(loc), "There is no need to spill unoccupied registers");
    ASSERT_MSG(!IsRegisterAllocated(loc), "Registers that have been allocated must not be spilt");

    HostLoc new_loc = FindFreeSpill();

    code->MOV(32, SpillToOpArg(new_loc), Gen::R(hostloc_to_x64.at(loc)));

    hostloc_to_value[new_loc] = hostloc_to_value[loc];
    hostloc_to_value[loc] = nullptr;
}

HostLoc RegAlloc::FindFreeSpill() const {
    for (size_t i = 0; i < SpillCount; i++)
        if (!IsRegisterOccupied(HostLocSpill(i)))
            return HostLocSpill(i);

    ASSERT_MSG(0, "All spill locations are full");
}

void RegAlloc::EndOfAllocScope() {
    hostloc_state.clear();

    for (auto& iter : hostloc_to_value)
        if (iter.second && remaining_uses[iter.second] == 0)
            iter.second = nullptr;
}

void RegAlloc::DecrementRemainingUses(IR::Value* value) {
    ASSERT_MSG(remaining_uses.find(value) != remaining_uses.end(), "value does not exist");
    ASSERT_MSG(remaining_uses[value] > 0, "value doesn't have any remaining uses");
    remaining_uses[value]--;
}

void RegAlloc::AssertNoMoreUses() {
    ASSERT(std::all_of(hostloc_to_value.begin(), hostloc_to_value.end(), [](const auto& pair){ return !pair.second; }));
}

void RegAlloc::Reset() {
    hostloc_to_value.clear();
    hostloc_state.clear();
    remaining_uses.clear();
}

} // namespace BackendX64
} // namespace Dynarmic
