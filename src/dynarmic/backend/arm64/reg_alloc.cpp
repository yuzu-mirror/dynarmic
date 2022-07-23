/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/arm64/reg_alloc.h"

#include <algorithm>
#include <array>

#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

constexpr size_t spill_offset = offsetof(StackLayout, spill);
constexpr size_t spill_slot_size = sizeof(decltype(StackLayout::spill)::value_type);

static bool IsValuelessType(IR::Type type) {
    switch (type) {
    case IR::Type::Table:
        return true;
    default:
        return false;
    }
}

IR::Type Argument::GetType() const {
    return value.GetType();
}

bool Argument::IsImmediate() const {
    return value.IsImmediate();
}

bool Argument::GetImmediateU1() const {
    return value.GetU1();
}

u8 Argument::GetImmediateU8() const {
    const u64 imm = value.GetImmediateAsU64();
    ASSERT(imm < 0x100);
    return u8(imm);
}

u16 Argument::GetImmediateU16() const {
    const u64 imm = value.GetImmediateAsU64();
    ASSERT(imm < 0x10000);
    return u16(imm);
}

u32 Argument::GetImmediateU32() const {
    const u64 imm = value.GetImmediateAsU64();
    ASSERT(imm < 0x100000000);
    return u32(imm);
}

u64 Argument::GetImmediateU64() const {
    return value.GetImmediateAsU64();
}

IR::Cond Argument::GetImmediateCond() const {
    ASSERT(IsImmediate() && GetType() == IR::Type::Cond);
    return value.GetCond();
}

IR::AccType Argument::GetImmediateAccType() const {
    ASSERT(IsImmediate() && GetType() == IR::Type::AccType);
    return value.GetAccType();
}

HostLoc::Kind Argument::CurrentLocationKind() const {
    return reg_alloc.ValueLocation(value.GetInst())->kind;
}

bool HostLocInfo::Contains(const IR::Inst* value) const {
    return std::find(values.begin(), values.end(), value) != values.end();
}

void HostLocInfo::SetupLocation(const IR::Inst* value) {
    values.clear();
    values.emplace_back(value);
    locked = true;
    realized = true;
    uses_this_inst = 0;
    accumulated_uses = 0;
    expected_uses = value->UseCount();
}

bool HostLocInfo::IsCompletelyEmpty() const {
    return values.empty() && !locked && !realized && !accumulated_uses && !expected_uses && !uses_this_inst;
}

bool HostLocInfo::IsImmediatelyAllocatable() const {
    return values.empty() && !locked;
}

bool HostLocInfo::IsOneRemainingUse() const {
    return accumulated_uses + 1 == expected_uses && uses_this_inst == 1;
}

RegAlloc::ArgumentInfo RegAlloc::GetArgumentInfo(IR::Inst* inst) {
    ArgumentInfo ret = {Argument{*this}, Argument{*this}, Argument{*this}, Argument{*this}};
    for (size_t i = 0; i < inst->NumArgs(); i++) {
        const IR::Value arg = inst->GetArg(i);
        ret[i].value = arg;
        if (!arg.IsImmediate() && !IsValuelessType(arg.GetType())) {
            ASSERT_MSG(ValueLocation(arg.GetInst()), "argument must already been defined");
            ValueInfo(arg.GetInst()).uses_this_inst++;
        }
    }
    return ret;
}

bool RegAlloc::IsValueLive(IR::Inst* inst) const {
    return !!ValueLocation(inst);
}

void RegAlloc::DefineAsExisting(IR::Inst* inst, Argument& arg) {
    auto& info = ValueInfo(arg.value.GetInst());
    info.values.emplace_back(inst);
    info.expected_uses += inst->UseCount();
}

void RegAlloc::AssertNoMoreUses() const {
    const auto is_empty = [](const auto& i) { return i.IsCompletelyEmpty(); };
    ASSERT(std::all_of(gprs.begin(), gprs.end(), is_empty));
    ASSERT(std::all_of(fprs.begin(), fprs.end(), is_empty));
    ASSERT(is_empty(flags));
    ASSERT(std::all_of(spills.begin(), spills.end(), is_empty));
}

template<HostLoc::Kind required_kind>
int RegAlloc::RealizeReadImpl(const IR::Inst* value) {
    const auto current_location = ValueLocation(value);
    ASSERT(current_location);

    if (current_location->kind == required_kind) {
        ValueInfo(*current_location).realized = true;
        return current_location->index;
    }

    ASSERT(!ValueInfo(*current_location).realized);
    ASSERT(ValueInfo(*current_location).locked);

    if constexpr (required_kind == HostLoc::Kind::Gpr) {
        const int new_location_index = AllocateRegister(gprs, gpr_order);
        SpillGpr(new_location_index);

        switch (current_location->kind) {
        case HostLoc::Kind::Gpr:
            ASSERT_FALSE("Logic error");
            break;
        case HostLoc::Kind::Fpr:
            code.FMOV(oaknut::XReg{current_location->index}, oaknut::DReg{new_location_index});
            // ASSERT size fits
            break;
        case HostLoc::Kind::Spill:
            code.LDR(oaknut::XReg{new_location_index}, SP, spill_offset + new_location_index * spill_slot_size);
            break;
        case HostLoc::Kind::Flags:
            code.MRS(oaknut::XReg{new_location_index}, static_cast<oaknut::SystemReg>(0b11'011'0100'0010'000));
            break;
        }

        gprs[new_location_index] = std::exchange(ValueInfo(*current_location), {});
        gprs[new_location_index].realized = true;
        return new_location_index;
    } else if constexpr (required_kind == HostLoc::Kind::Fpr) {
        const int new_location_index = AllocateRegister(fprs, fpr_order);
        SpillFpr(new_location_index);

        switch (current_location->kind) {
        case HostLoc::Kind::Gpr:
            code.FMOV(oaknut::DReg{new_location_index}, oaknut::XReg{current_location->index});
            break;
        case HostLoc::Kind::Fpr:
            ASSERT_FALSE("Logic error");
            break;
        case HostLoc::Kind::Spill:
            code.LDR(oaknut::QReg{new_location_index}, SP, spill_offset + new_location_index * spill_slot_size);
            break;
        case HostLoc::Kind::Flags:
            ASSERT_FALSE("Moving from flags into fprs is not currently supported");
            break;
        }

        fprs[new_location_index] = std::exchange(ValueInfo(*current_location), {});
        fprs[new_location_index].realized = true;
        return new_location_index;
    } else if constexpr (required_kind == HostLoc::Kind::Flags) {
        ASSERT_FALSE("Loading flags back into NZCV is not currently supported");
    } else {
        static_assert(required_kind == HostLoc::Kind::Fpr || required_kind == HostLoc::Kind::Gpr || required_kind == HostLoc::Kind::Flags);
    }
}

template<HostLoc::Kind kind>
int RegAlloc::RealizeWriteImpl(const IR::Inst* value) {
    ASSERT(!ValueLocation(value));

    if constexpr (kind == HostLoc::Kind::Gpr) {
        const int new_location_index = AllocateRegister(gprs, gpr_order);
        SpillGpr(new_location_index);
        gprs[new_location_index].SetupLocation(value);
        return new_location_index;
    } else if constexpr (kind == HostLoc::Kind::Fpr) {
        const int new_location_index = AllocateRegister(fprs, fpr_order);
        SpillFpr(new_location_index);
        fprs[new_location_index].SetupLocation(value);
        return new_location_index;
    } else if constexpr (kind == HostLoc::Kind::Flags) {
        ASSERT(flags.values.empty());
        flags.SetupLocation(value);
        return 0;
    } else {
        static_assert(kind == HostLoc::Kind::Fpr || kind == HostLoc::Kind::Gpr || kind == HostLoc::Kind::Flags);
    }
}

template int RegAlloc::RealizeReadImpl<HostLoc::Kind::Gpr>(const IR::Inst* value);
template int RegAlloc::RealizeReadImpl<HostLoc::Kind::Fpr>(const IR::Inst* value);
template int RegAlloc::RealizeReadImpl<HostLoc::Kind::Flags>(const IR::Inst* value);
template int RegAlloc::RealizeWriteImpl<HostLoc::Kind::Gpr>(const IR::Inst* value);
template int RegAlloc::RealizeWriteImpl<HostLoc::Kind::Fpr>(const IR::Inst* value);
template int RegAlloc::RealizeWriteImpl<HostLoc::Kind::Flags>(const IR::Inst* value);

void RegAlloc::Unlock(HostLoc host_loc) {
    HostLocInfo& info = ValueInfo(host_loc);
    if (!info.realized) {
        return;
    }

    info.accumulated_uses += info.uses_this_inst;
    info.uses_this_inst = 0;

    if (info.accumulated_uses == info.expected_uses) {
        info = {};
    } else {
        info.realized = false;
        info.locked = false;
    }
}

int RegAlloc::AllocateRegister(const std::array<HostLocInfo, 32>& regs, const std::vector<int>& order) const {
    const auto empty = std::find_if(order.begin(), order.end(), [&](int i) { return regs[i].IsImmediatelyAllocatable(); });
    if (empty != order.end()) {
        return *empty;
    }

    std::vector<int> candidates;
    std::copy_if(order.begin(), order.end(), std::back_inserter(candidates), [&](int i) { return !regs[i].locked; });

    // TODO: LRU
    std::uniform_int_distribution<size_t> dis{0, candidates.size() - 1};
    return candidates[dis(rand_gen)];
}

void RegAlloc::SpillGpr(int index) {
    ASSERT(!gprs[index].locked && !gprs[index].realized);
    if (gprs[index].values.empty()) {
        return;
    }
    const int new_location_index = FindFreeSpill();
    code.STR(oaknut::XReg{index}, SP, spill_offset + new_location_index * spill_slot_size);
    spills[new_location_index] = std::exchange(gprs[index], {});
}

void RegAlloc::SpillFpr(int index) {
    ASSERT(!fprs[index].locked && !fprs[index].realized);
    if (fprs[index].values.empty()) {
        return;
    }
    const int new_location_index = FindFreeSpill();
    code.STR(oaknut::QReg{index}, SP, spill_offset + new_location_index * spill_slot_size);
    spills[new_location_index] = std::exchange(fprs[index], {});
}

void RegAlloc::SpillFlags() {
    ASSERT(!flags.locked && !flags.realized);
    if (flags.values.empty()) {
        return;
    }
    const int new_location_index = AllocateRegister(gprs, gpr_order);
    SpillGpr(new_location_index);
    code.MRS(oaknut::XReg{new_location_index}, static_cast<oaknut::SystemReg>(0b11'011'0100'0010'000));
    gprs[new_location_index] = std::exchange(flags, {});
}

int RegAlloc::FindFreeSpill() const {
    const auto iter = std::find_if(spills.begin(), spills.end(), [](const HostLocInfo& info) { return info.values.empty(); });
    ASSERT_MSG(iter != spills.end(), "All spill locations are full");
    return static_cast<int>(iter - spills.begin());
}

std::optional<HostLoc> RegAlloc::ValueLocation(const IR::Inst* value) const {
    const auto contains_value = [value](const HostLocInfo& info) { return info.Contains(value); };

    if (const auto iter = std::find_if(gprs.begin(), gprs.end(), contains_value); iter != gprs.end()) {
        return HostLoc{HostLoc::Kind::Gpr, static_cast<int>(iter - gprs.begin())};
    }
    if (const auto iter = std::find_if(fprs.begin(), fprs.end(), contains_value); iter != fprs.end()) {
        return HostLoc{HostLoc::Kind::Fpr, static_cast<int>(iter - fprs.begin())};
    }
    if (contains_value(flags)) {
        return HostLoc{HostLoc::Kind::Flags, 0};
    }
    if (const auto iter = std::find_if(spills.begin(), spills.end(), contains_value); iter != spills.end()) {
        return HostLoc{HostLoc::Kind::Spill, static_cast<int>(iter - spills.begin())};
    }
    return std::nullopt;
}

HostLocInfo& RegAlloc::ValueInfo(HostLoc host_loc) {
    switch (host_loc.kind) {
    case HostLoc::Kind::Gpr:
        return gprs[static_cast<size_t>(host_loc.index)];
    case HostLoc::Kind::Fpr:
        return fprs[static_cast<size_t>(host_loc.index)];
    case HostLoc::Kind::Flags:
        return flags;
    case HostLoc::Kind::Spill:
        return spills[static_cast<size_t>(host_loc.index)];
    }
    ASSERT_FALSE("RegAlloc::ValueInfo: Invalid HostLoc::Kind");
}

HostLocInfo& RegAlloc::ValueInfo(const IR::Inst* value) {
    const auto contains_value = [value](const HostLocInfo& info) { return info.Contains(value); };

    if (const auto iter = std::find_if(gprs.begin(), gprs.end(), contains_value); iter != gprs.end()) {
        return *iter;
    }
    if (const auto iter = std::find_if(fprs.begin(), fprs.end(), contains_value); iter != gprs.end()) {
        return *iter;
    }
    if (contains_value(flags)) {
        return flags;
    }
    if (const auto iter = std::find_if(spills.begin(), spills.end(), contains_value); iter != gprs.end()) {
        return *iter;
    }
    ASSERT_FALSE("RegAlloc::ValueInfo: Value not found");
}

}  // namespace Dynarmic::Backend::Arm64
