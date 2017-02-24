/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <algorithm>

#include <xbyak.h>

#include "backend_x64/abi.h"
#include "backend_x64/jitstate.h"
#include "backend_x64/reg_alloc.h"
#include "common/assert.h"

namespace Dynarmic {
namespace BackendX64 {

static u64 ImmediateToU64(const IR::Value& imm) {
    switch (imm.GetType()) {
    case IR::Type::U1:
        return u64(imm.GetU1());
    case IR::Type::U8:
        return u64(imm.GetU8());
    case IR::Type::U16:
        return u64(imm.GetU16());
    case IR::Type::U32:
        return u64(imm.GetU32());
    case IR::Type::U64:
        return u64(imm.GetU64());
    default:
        ASSERT_MSG(false, "This should never happen.");
    }
}

static Xbyak::Reg HostLocToX64(HostLoc hostloc) {
    if (HostLocIsGPR(hostloc)) {
        DEBUG_ASSERT(hostloc != HostLoc::RSP && hostloc != HostLoc::R15);
        return HostLocToReg64(hostloc);
    }
    if (HostLocIsXMM(hostloc)) {
        return HostLocToXmm(hostloc);
    }
    ASSERT_MSG(false, "This should never happen.");
}

static bool IsSameHostLocClass(HostLoc a, HostLoc b) {
    return (HostLocIsGPR(a) && HostLocIsGPR(b))
           || (HostLocIsXMM(a) && HostLocIsXMM(b))
           || (HostLocIsSpill(a) && HostLocIsSpill(b));
}

static void EmitMove(BlockOfCode* code, HostLoc to, HostLoc from) {
    if (HostLocIsXMM(to) && HostLocIsXMM(from)) {
        code->movaps(HostLocToXmm(to), HostLocToXmm(from));
    } else if (HostLocIsGPR(to) && HostLocIsGPR(from)) {
        code->mov(HostLocToReg64(to), HostLocToReg64(from));
    } else if (HostLocIsXMM(to) && HostLocIsGPR(from)) {
        code->movq(HostLocToXmm(to), HostLocToReg64(from));
    } else if (HostLocIsGPR(to) && HostLocIsXMM(from)) {
        code->movq(HostLocToReg64(to), HostLocToXmm(from));
    } else if (HostLocIsXMM(to) && HostLocIsSpill(from)) {
        code->movsd(HostLocToXmm(to), SpillToOpArg(from));
    } else if (HostLocIsSpill(to) && HostLocIsXMM(from)) {
        code->movsd(SpillToOpArg(to), HostLocToXmm(from));
    } else if (HostLocIsGPR(to) && HostLocIsSpill(from)) {
        code->mov(HostLocToReg64(to), SpillToOpArg(from));
    } else if (HostLocIsSpill(to) && HostLocIsGPR(from)) {
        code->mov(SpillToOpArg(to), HostLocToReg64(from));
    } else {
        ASSERT_MSG(false, "Invalid RegAlloc::EmitMove");
    }
}

static void EmitExchange(BlockOfCode* code, HostLoc a, HostLoc b) {
    if (HostLocIsGPR(a) && HostLocIsGPR(b)) {
        code->xchg(HostLocToReg64(a), HostLocToReg64(b));
    } else if (HostLocIsXMM(a) && HostLocIsXMM(b)) {
        ASSERT_MSG(false, "Check your code: Exchanging XMM registers is unnecessary");
    } else {
        ASSERT_MSG(false, "Invalid RegAlloc::EmitExchange");
    }
}

bool Argument::GetImmediateU1() const {
    return value.GetU1();
}

u8 Argument::GetImmediateU8() const {
    u64 imm = ImmediateToU64(value);
    ASSERT(imm < 0x100);
    return u8(imm);
}

u16 Argument::GetImmediateU16() const {
    u64 imm = ImmediateToU64(value);
    ASSERT(imm < 0x10000);
    return u16(imm);
}

u32 Argument::GetImmediateU32() const {
    u64 imm = ImmediateToU64(value);
    ASSERT(imm < 0x100000000);
    return u32(imm);
}

u64 Argument::GetImmediateU64() const {
    return ImmediateToU64(value);
}

bool Argument::IsInGpr() const {
    return HostLocIsGPR(*reg_alloc.ValueLocation(value.GetInst()));
}

bool Argument::IsInXmm() const {
    return HostLocIsXMM(*reg_alloc.ValueLocation(value.GetInst()));
}

bool Argument::IsInMemory() const {
    return HostLocIsSpill(*reg_alloc.ValueLocation(value.GetInst()));
}

std::array<Argument, 3> RegAlloc::GetArgumentInfo(IR::Inst* inst) {
    std::array<Argument, 3> ret = { Argument{*this}, Argument{*this}, Argument{*this}};
    for (size_t i = 0; i < inst->NumArgs(); i++) {
        IR::Value arg = inst->GetArg(i);
        ret[i].value = arg;
    }
    return ret;
}

void RegAlloc::RegisterAddDef(IR::Inst* def_inst, const IR::Value& use_inst) {
    DEBUG_ASSERT_MSG(!ValueLocation(def_inst), "def_inst has already been defined");

    if (use_inst.IsImmediate()) {
        HostLoc location = ScratchHostLocReg(any_gpr);
        DefineValue(def_inst, location);
        LoadImmediateIntoHostLocReg(use_inst, location);
        return;
    }

    use_inst.GetInst()->DecrementRemainingUses();
    DEBUG_ASSERT_MSG(ValueLocation(use_inst.GetInst()), "use_inst must already be defined");
    HostLoc location = *ValueLocation(use_inst.GetInst());
    DefineValue(def_inst, location);
}

std::tuple<OpArg, HostLoc> RegAlloc::UseDefOpArgHostLocReg(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations) {
    DEBUG_ASSERT(std::all_of(desired_locations.begin(), desired_locations.end(), HostLocIsRegister));
    DEBUG_ASSERT_MSG(!ValueLocation(def_inst), "def_inst has already been defined");
    DEBUG_ASSERT_MSG(use_value.IsImmediate() || ValueLocation(use_value.GetInst()), "use_inst has not been defined");

    // TODO: IsLastUse optimization

    OpArg use_oparg = UseOpArg(use_value, any_gpr);
    HostLoc def_reg = ScratchHostLocReg(desired_locations);
    DefineValue(def_inst, def_reg);
    return std::make_tuple(use_oparg, def_reg);
}

HostLoc RegAlloc::UseHostLocReg(IR::Value use_value, HostLocList desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseHostLocReg(use_value.GetInst(), desired_locations);
    }

    return LoadImmediateIntoHostLocReg(use_value, ScratchHostLocReg(desired_locations));
}

HostLoc RegAlloc::UseHostLocReg(IR::Inst* use_inst, HostLocList desired_locations) {
    use_inst->DecrementRemainingUses();

    const HostLoc current_location = *ValueLocation(use_inst);

    const bool can_use_current_location = std::find(desired_locations.begin(), desired_locations.end(), current_location) != desired_locations.end();
    if (can_use_current_location) {
        LocInfo(current_location).ReadLock();
        return current_location;
    }

    if (LocInfo(current_location).IsLocked()) {
        return UseScratchHostLocReg(use_inst, desired_locations);
    }

    const HostLoc destination_location = SelectARegister(desired_locations);
    if (IsSameHostLocClass(destination_location, current_location)) {
        Exchange(destination_location, current_location);
    } else {
        MoveOutOfTheWay(destination_location);
        Move(destination_location, current_location);
    }
    LocInfo(destination_location).ReadLock();
    return destination_location;
}

OpArg RegAlloc::UseOpArg(IR::Value use_value, HostLocList desired_locations) {
    if (use_value.IsImmediate()) {
        ASSERT_MSG(false, "UseOpArg does not support immediates");
        return {}; // return a None
    }

    // TODO: Reimplement properly
    return HostLocToX64(UseHostLocReg(use_value.GetInst(), desired_locations));
}

HostLoc RegAlloc::UseScratchHostLocReg(IR::Value use_value, HostLocList desired_locations) {
    if (!use_value.IsImmediate()) {
        return UseScratchHostLocReg(use_value.GetInst(), desired_locations);
    }

    return LoadImmediateIntoHostLocReg(use_value, ScratchHostLocReg(desired_locations));
}

HostLoc RegAlloc::UseScratchHostLocReg(IR::Inst* use_inst, HostLocList desired_locations) {
    use_inst->DecrementRemainingUses();

    const HostLoc current_location = *ValueLocation(use_inst);

    const bool can_use_current_location = std::find(desired_locations.begin(), desired_locations.end(), current_location) != desired_locations.end();
    if (can_use_current_location && !LocInfo(current_location).IsLocked()) {
        MoveOutOfTheWay(current_location);
        LocInfo(current_location).WriteLock();
        return current_location;
    }

    const HostLoc destination_location = SelectARegister(desired_locations);
    MoveOutOfTheWay(destination_location);
    CopyToScratch(destination_location, current_location);
    LocInfo(destination_location).WriteLock();
    return destination_location;
}

HostLoc RegAlloc::ScratchHostLocReg(HostLocList desired_locations) {
    HostLoc location = SelectARegister(desired_locations);
    MoveOutOfTheWay(location);
    LocInfo(location).WriteLock();
    return location;
}

void RegAlloc::HostCall(IR::Inst* result_def, IR::Value arg0_use, IR::Value arg1_use, IR::Value arg2_use, IR::Value arg3_use) {
    constexpr size_t args_count = 4;
    constexpr std::array<HostLoc, args_count> args_hostloc = { ABI_PARAM1, ABI_PARAM2, ABI_PARAM3, ABI_PARAM4 };
    const std::array<IR::Value*, args_count> args = {&arg0_use, &arg1_use, &arg2_use, &arg3_use};

    const static std::vector<HostLoc> other_caller_save = [args_hostloc](){
        std::vector<HostLoc> ret(ABI_ALL_CALLER_SAVE.begin(), ABI_ALL_CALLER_SAVE.end());

        for (auto hostloc : args_hostloc)
            ret.erase(std::find(ret.begin(), ret.end(), hostloc));

        return ret;
    }();

    // TODO: This works but almost certainly leads to suboptimal generated code.

    if (result_def) {
        DefineValue(result_def, ScratchHostLocReg({ABI_RETURN}));
    } else {
        ScratchHostLocReg({ABI_RETURN});
    }

    for (size_t i = 0; i < args_count; i++) {
        if (!args[i]->IsEmpty()) {
            UseScratchHostLocReg(*args[i], {args_hostloc[i]});
        } else {
            ScratchHostLocReg({args_hostloc[i]});
        }
    }

    for (HostLoc caller_saved : other_caller_save) {
        ScratchHostLocReg({caller_saved});
    }
}

HostLoc RegAlloc::SelectARegister(HostLocList desired_locations) const {
    std::vector<HostLoc> candidates = desired_locations;

    // Find all locations that have not been allocated..
    auto allocated_locs = std::partition(candidates.begin(), candidates.end(), [this](auto loc){
        return !this->LocInfo(loc).IsLocked();
    });
    candidates.erase(allocated_locs, candidates.end());
    ASSERT_MSG(!candidates.empty(), "All candidate registers have already been allocated");

    // Selects the best location out of the available locations.
    // TODO: Actually do LRU or something. Currently we just try to pick something without a value if possible.

    std::partition(candidates.begin(), candidates.end(), [this](auto loc){
        return this->LocInfo(loc).IsEmpty();
    });

    return candidates.front();
}

boost::optional<HostLoc> RegAlloc::ValueLocation(const IR::Inst* value) const {
    for (size_t i = 0; i < HostLocCount; i++)
        if (hostloc_info[i].ContainsValue(value))
            return boost::make_optional<HostLoc>(static_cast<HostLoc>(i));

    return boost::none;
}

void RegAlloc::DefineValue(IR::Inst* def_inst, HostLoc host_loc) {
    DEBUG_ASSERT_MSG(!ValueLocation(def_inst), "def_inst has already been defined");
    LocInfo(host_loc).AddValue(def_inst);
}

void RegAlloc::SpillRegister(HostLoc loc) {
    ASSERT_MSG(HostLocIsRegister(loc), "Only registers can be spilled");
    ASSERT_MSG(!LocInfo(loc).IsEmpty(), "There is no need to spill unoccupied registers");
    ASSERT_MSG(!LocInfo(loc).IsLocked(), "Registers that have been allocated must not be spilt");

    HostLoc new_loc = FindFreeSpill();
    Move(new_loc, loc);
}

HostLoc RegAlloc::FindFreeSpill() const {
    for (size_t i = 0; i < SpillCount; i++)
        if (LocInfo(HostLocSpill(i)).IsEmpty())
            return HostLocSpill(i);

    ASSERT_MSG(false, "All spill locations are full");
}

void RegAlloc::EndOfAllocScope() {
    for (auto& iter : hostloc_info) {
        iter.EndOfAllocScope();
    }
}

void RegAlloc::AssertNoMoreUses() {
    if (!std::all_of(hostloc_info.begin(), hostloc_info.end(), [](const auto& i){ return i.IsEmpty(); })) {
        ASSERT_MSG(false, "bad");
    }
}

void RegAlloc::Reset() {
    hostloc_info.fill({});
}

HostLoc RegAlloc::LoadImmediateIntoHostLocReg(IR::Value imm, HostLoc host_loc) {
    ASSERT_MSG(imm.IsImmediate(), "imm is not an immediate");

    Xbyak::Reg64 reg = HostLocToReg64(host_loc);

    u64 imm_value = ImmediateToU64(imm);
    if (imm_value == 0)
        code->xor_(reg.cvt32(), reg.cvt32());
    else
        code->mov(reg, imm_value);
    return host_loc;
}

void RegAlloc::Move(HostLoc to, HostLoc from) {
    ASSERT(LocInfo(to).IsEmpty() && !LocInfo(from).IsLocked());

    if (LocInfo(from).IsEmpty()) {
        return;
    }

    LocInfo(to) = LocInfo(from);
    LocInfo(from) = {};

    EmitMove(code, to, from);
}

void RegAlloc::CopyToScratch(HostLoc to, HostLoc from) {
    ASSERT(LocInfo(to).IsEmpty() && !LocInfo(from).IsEmpty());

    EmitMove(code, to, from);
}

void RegAlloc::Exchange(HostLoc a, HostLoc b) {
    ASSERT(!LocInfo(a).IsLocked() && !LocInfo(b).IsLocked());

    if (LocInfo(a).IsEmpty()) {
        Move(a, b);
        return;
    }

    if (LocInfo(b).IsEmpty()) {
        Move(b, a);
        return;
    }

    std::swap(LocInfo(a), LocInfo(b));

    EmitExchange(code, a, b);
}

void RegAlloc::MoveOutOfTheWay(HostLoc reg) {
    ASSERT(!LocInfo(reg).IsLocked());
    if (!LocInfo(reg).IsEmpty()) {
        SpillRegister(reg);
    }
}

} // namespace BackendX64
} // namespace Dynarmic
