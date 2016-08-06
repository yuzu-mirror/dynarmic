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

static Gen::OpArg ImmediateToOpArg(const IR::Value& imm) {
    switch (imm.GetType()) {
        case IR::Type::U1:
            return Gen::Imm32(imm.GetU1());
            break;
        case IR::Type::U8:
            return Gen::Imm32(imm.GetU8());
            break;
        case IR::Type::U32:
            return Gen::Imm32(imm.GetU32());
            break;
        default:
            ASSERT_MSG(false, "This should never happen.");
    }
}

static Gen::X64Reg HostLocToX64(HostLoc loc) {
    DEBUG_ASSERT(HostLocIsRegister(loc));
    // HostLoc is ordered such that the numbers line up.
    if (HostLocIsGPR(loc)) {
        return static_cast<Gen::X64Reg>(loc);
    }
    if (HostLocIsXMM(loc)) {
        return static_cast<Gen::X64Reg>(size_t(loc) - size_t(HostLoc::XMM0));
    }
    ASSERT_MSG(false, "This should never happen.");
    return Gen::INVALID_REG;
}

static Gen::OpArg SpillToOpArg(HostLoc loc) {
    static_assert(std::is_same<decltype(JitState{}.Spill[0]), u64&>::value, "Spill must be u64");
    DEBUG_ASSERT(HostLocIsSpill(loc));

    size_t i = static_cast<size_t>(loc) - static_cast<size_t>(HostLoc::FirstSpill);
    return Gen::MDisp(Gen::R15, static_cast<int>(offsetof(JitState, Spill) + i * sizeof(u64)));
}

Gen::X64Reg RegAlloc::DefRegister(IR::Inst* def_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(!ValueLocation(def_inst), "def_inst has already been defined");

    HostLoc location = SelectARegister(desired_locations);

    if (IsRegisterOccupied(location)) {
        SpillRegister(location);
    }

    LocInfo(location).is_being_used = true;
    LocInfo(location).def = def_inst;

    DEBUG_ASSERT(LocInfo(location).IsDef());
    return HostLocToX64(location);
}

void RegAlloc::RegisterAddDef(IR::Inst* def_inst, const IR::Value& use_inst) {
    DEBUG_ASSERT_MSG(!ValueLocation(def_inst), "def_inst has already been defined");

    if (use_inst.IsImmediate()) {
        LoadImmediateIntoRegister(use_inst, DefRegister(def_inst, any_gpr));
        return;
    }

    DEBUG_ASSERT_MSG(ValueLocation(use_inst.GetInst()), "use_inst must already be defined");
    HostLoc location = *ValueLocation(use_inst.GetInst());
    LocInfo(location).values.emplace_back(def_inst);
    DecrementRemainingUses(use_inst.GetInst());
    DEBUG_ASSERT(LocInfo(location).IsIdle());
}

Gen::X64Reg RegAlloc::UseDefRegister(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseDefRegister(use_value.GetInst(), def_inst, desired_locations);
    }

    return LoadImmediateIntoRegister(use_value, DefRegister(def_inst, desired_locations));
}

Gen::X64Reg RegAlloc::UseDefRegister(IR::Inst* use_inst, IR::Inst* def_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(!ValueLocation(def_inst), "def_inst has already been defined");
    DEBUG_ASSERT_MSG(ValueLocation(use_inst), "use_inst has not been defined");

    if (IsLastUse(use_inst)) {
        HostLoc current_location = *ValueLocation(use_inst);
        auto& loc_info = LocInfo(current_location);
        if (loc_info.IsIdle()) {
            loc_info.is_being_used = true;
            loc_info.def = def_inst;
            DEBUG_ASSERT(loc_info.IsUseDef());
            if (HostLocIsSpill(current_location)) {
                HostLoc new_location = SelectARegister(desired_locations);
                if (IsRegisterOccupied(new_location)) {
                    SpillRegister(new_location);
                }
                EmitMove(new_location, current_location);
                LocInfo(new_location) = LocInfo(current_location);
                LocInfo(current_location) = {};
                return HostLocToX64(new_location);
            } else {
                return HostLocToX64(current_location);
            }
        }
    }

    bool is_floating_point = use_inst->GetType() == IR::Type::F32 || use_inst->GetType() == IR::Type::F64;
    Gen::X64Reg use_reg = UseRegister(use_inst, is_floating_point ? any_xmm : any_gpr);
    Gen::X64Reg def_reg = DefRegister(def_inst, desired_locations);
    if (is_floating_point) {
        code->MOVAPD(def_reg, Gen::R(use_reg));
    } else {
        code->MOV(64, Gen::R(def_reg), Gen::R(use_reg));
    }
    return def_reg;
}

std::tuple<Gen::OpArg, Gen::X64Reg> RegAlloc::UseDefOpArg(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(!ValueLocation(def_inst), "def_inst has already been defined");
    DEBUG_ASSERT_MSG(use_value.IsImmediate() || ValueLocation(use_value.GetInst()), "use_inst has not been defined");

    if (!use_value.IsImmediate()) {
        IR::Inst* use_inst = use_value.GetInst();

        if (IsLastUse(use_inst)) {
            HostLoc current_location = *ValueLocation(use_inst);
            auto& loc_info = LocInfo(current_location);
            if (!loc_info.IsIdle()) {
                if (HostLocIsSpill(current_location)) {
                    loc_info.is_being_used = true;
                    DEBUG_ASSERT(loc_info.IsUse());
                    return std::make_tuple(SpillToOpArg(current_location), DefRegister(def_inst, desired_locations));
                } else {
                    loc_info.is_being_used = true;
                    loc_info.def = def_inst;
                    DEBUG_ASSERT(loc_info.IsUseDef());
                    return std::make_tuple(Gen::R(HostLocToX64(current_location)), HostLocToX64(current_location));
                }
            }
        }
    }

    Gen::OpArg use_oparg = UseOpArg(use_value, any_gpr);
    Gen::X64Reg def_reg = DefRegister(def_inst, desired_locations);
    return std::make_tuple(use_oparg, def_reg);
}

Gen::X64Reg RegAlloc::UseRegister(IR::Value use_value, HostLocList desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseRegister(use_value.GetInst(), desired_locations);
    }

    return LoadImmediateIntoRegister(use_value, ScratchRegister(desired_locations));
}

Gen::X64Reg RegAlloc::UseRegister(IR::Inst* use_inst, HostLocList desired_locations) {
    HostLoc current_location;
    bool was_being_used;
    std::tie(current_location, was_being_used) = UseHostLoc(use_inst, desired_locations);

    if (HostLocIsRegister(current_location)) {
        return HostLocToX64(current_location);
    } else if (HostLocIsSpill(current_location)) {
        HostLoc new_location = SelectARegister(desired_locations);
        if (IsRegisterOccupied(new_location)) {
            SpillRegister(new_location);
        }
        EmitMove(new_location, current_location);
        if (!was_being_used) {
            LocInfo(new_location) = LocInfo(current_location);
            LocInfo(current_location) = {};
            DEBUG_ASSERT(LocInfo(new_location).IsUse());
        } else {
            LocInfo(new_location).is_being_used = true;
            DEBUG_ASSERT(LocInfo(new_location).IsScratch());
        }
        return HostLocToX64(new_location);
    }

    ASSERT_MSG(false, "Unknown current_location type");
    return Gen::INVALID_REG;
}

Gen::OpArg RegAlloc::UseOpArg(IR::Value use_value, HostLocList desired_locations) {
    if (use_value.IsImmediate()) {
        return ImmediateToOpArg(use_value);
    }

    IR::Inst* use_inst = use_value.GetInst();

    HostLoc current_location;
    bool was_being_used;
    std::tie(current_location, was_being_used) = UseHostLoc(use_inst, desired_locations);

    if (HostLocIsRegister(current_location)) {
        return Gen::R(HostLocToX64(current_location));
    } else if (HostLocIsSpill(current_location)) {
        return SpillToOpArg(current_location);
    }

    ASSERT_MSG(false, "Unknown current_location type");
    return Gen::R(Gen::INVALID_REG);
}

Gen::X64Reg RegAlloc::UseScratchRegister(IR::Value use_value, HostLocList desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseScratchRegister(use_value.GetInst(), desired_locations);
    }

    return LoadImmediateIntoRegister(use_value, ScratchRegister(desired_locations));
}

Gen::X64Reg RegAlloc::UseScratchRegister(IR::Inst* use_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(ValueLocation(use_inst), "use_inst has not been defined");
    ASSERT_MSG(use_inst->use_count != 0, "use_inst ran out of uses. (Use-d an IR::Inst* too many times)");

    HostLoc current_location = *ValueLocation(use_inst);
    HostLoc new_location = SelectARegister(desired_locations);
    if (IsRegisterOccupied(new_location)) {
        SpillRegister(new_location);
    }

    if (HostLocIsSpill(current_location)) {
        EmitMove(new_location, current_location);
        LocInfo(new_location).is_being_used = true;
        DecrementRemainingUses(use_inst);
        DEBUG_ASSERT(LocInfo(new_location).IsScratch());
        return HostLocToX64(new_location);
    } else if (HostLocIsRegister(current_location)) {
        ASSERT(LocInfo(current_location).IsIdle());

        if (current_location != new_location) {
            EmitMove(new_location, current_location);
        }

        LocInfo(new_location).is_being_used = true;
        LocInfo(new_location).values.clear();
        DecrementRemainingUses(use_inst);
        DEBUG_ASSERT(LocInfo(new_location).IsScratch());
        return HostLocToX64(new_location);
    }

    ASSERT_MSG(0, "Invalid current_location");
    return Gen::INVALID_REG;
}

Gen::X64Reg RegAlloc::ScratchRegister(HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));

    HostLoc location = SelectARegister(desired_locations);

    if (IsRegisterOccupied(location)) {
        SpillRegister(location);
    }

    // Update state
    LocInfo(location).is_being_used = true;

    DEBUG_ASSERT(LocInfo(location).IsScratch());
    return HostLocToX64(location);
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

boost::optional<HostLoc> RegAlloc::ValueLocation(IR::Inst* value) const {
    for (size_t i = 0; i < HostLocCount; i++)
        for (IR::Inst* v : hostloc_info[i].values)
            if (v == value)
                return boost::make_optional<HostLoc>(static_cast<HostLoc>(i));

    return boost::none;
}

bool RegAlloc::IsRegisterOccupied(HostLoc loc) const {
    return !GetLocInfo(loc).values.empty() || GetLocInfo(loc).def;
}

bool RegAlloc::IsRegisterAllocated(HostLoc loc) const {
    return GetLocInfo(loc).is_being_used;
}

bool RegAlloc::IsLastUse(IR::Inst* inst) const {
    if (inst->use_count > 1)
        return false;
    return GetLocInfo(*ValueLocation(inst)).values.size() == 1;
}

void RegAlloc::SpillRegister(HostLoc loc) {
    ASSERT_MSG(HostLocIsRegister(loc), "Only registers can be spilled");
    ASSERT_MSG(IsRegisterOccupied(loc), "There is no need to spill unoccupied registers");
    ASSERT_MSG(!IsRegisterAllocated(loc), "Registers that have been allocated must not be spilt");

    HostLoc new_loc = FindFreeSpill();

    EmitMove(new_loc, loc);

    LocInfo(new_loc) = LocInfo(loc);
    LocInfo(loc) = {};
}

HostLoc RegAlloc::FindFreeSpill() const {
    for (size_t i = 0; i < SpillCount; i++)
        if (!IsRegisterOccupied(HostLocSpill(i)))
            return HostLocSpill(i);

    ASSERT_MSG(0, "All spill locations are full");
}

void RegAlloc::EndOfAllocScope() {
    for (auto& iter : hostloc_info) {
        iter.is_being_used = false;
        if (iter.def) {
            iter.values.clear();
            iter.values.emplace_back(iter.def);
            iter.def = nullptr;
        } else if (!iter.values.empty()) {
            auto to_erase = std::remove_if(iter.values.begin(), iter.values.end(),
                                           [](const auto& inst){ return inst->use_count <= 0; });
            iter.values.erase(to_erase, iter.values.end());
        }
    }
}

void RegAlloc::DecrementRemainingUses(IR::Inst* value) {
    ASSERT_MSG(value->use_count > 0, "value doesn't have any remaining uses");
    value->use_count--;
}

void RegAlloc::AssertNoMoreUses() {
    ASSERT(std::all_of(hostloc_info.begin(), hostloc_info.end(), [](const auto& i){ return i.values.empty(); }));
}

void RegAlloc::Reset() {
    hostloc_info.fill({});
}

void RegAlloc::EmitMove(HostLoc to, HostLoc from) {
    if (HostLocIsXMM(to) && HostLocIsSpill(from)) {
        code->MOVSD(HostLocToX64(to), SpillToOpArg(from));
    } else if (HostLocIsSpill(to) && HostLocIsXMM(from)) {
        code->MOVSD(SpillToOpArg(to), HostLocToX64(from));
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

std::tuple<HostLoc, bool> RegAlloc::UseHostLoc(IR::Inst* use_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(ValueLocation(use_inst), "use_inst has not been defined");

    HostLoc current_location = *ValueLocation(use_inst);
    auto iter = std::find(desired_locations.begin(), desired_locations.end(), current_location);
    if (iter != desired_locations.end()) {
        if (LocInfo(current_location).IsDef()) {
            HostLoc new_location = SelectARegister(desired_locations);
            if (IsRegisterOccupied(new_location)) {
                SpillRegister(new_location);
            }
            EmitMove(new_location, current_location);
            LocInfo(new_location).is_being_used = true;
            LocInfo(new_location).values.emplace_back(use_inst);
            DecrementRemainingUses(use_inst);
            DEBUG_ASSERT(LocInfo(new_location).IsUse());
            return std::make_tuple(new_location, false);
        } else {
            bool was_being_used = LocInfo(current_location).is_being_used;
            ASSERT(LocInfo(current_location).IsUse() || LocInfo(current_location).IsIdle());
            LocInfo(current_location).is_being_used = true;
            DecrementRemainingUses(use_inst);
            DEBUG_ASSERT(LocInfo(current_location).IsUse());
            return std::make_tuple(current_location, was_being_used);
        }
    }

    if (HostLocIsSpill(current_location)) {
        bool was_being_used = LocInfo(current_location).is_being_used;
        LocInfo(current_location).is_being_used = true;
        DecrementRemainingUses(use_inst);
        DEBUG_ASSERT(LocInfo(current_location).IsUse());
        return std::make_tuple(current_location, was_being_used);
    } else if (HostLocIsRegister(current_location)) {
        HostLoc new_location = SelectARegister(desired_locations);
        ASSERT(LocInfo(current_location).IsIdle());
        EmitExchange(new_location, current_location);
        std::swap(LocInfo(new_location), LocInfo(current_location));
        LocInfo(new_location).is_being_used = true;
        DecrementRemainingUses(use_inst);
        DEBUG_ASSERT(LocInfo(new_location).IsUse());
        return std::make_tuple(new_location, false);
    }

    ASSERT_MSG(0, "Invalid current_location");
    return std::make_tuple(static_cast<HostLoc>(-1), false);
}

Gen::X64Reg RegAlloc::LoadImmediateIntoRegister(IR::Value imm, Gen::X64Reg reg) {
    ASSERT_MSG(imm.IsImmediate(), "imm is not an immediate");
    code->MOV(32, Gen::R(reg), ImmediateToOpArg(imm));
    return reg;
}

} // namespace BackendX64
} // namespace Dynarmic
