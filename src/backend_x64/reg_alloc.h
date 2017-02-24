/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <vector>

#include <boost/optional.hpp>
#include <xbyak.h>

#include "backend_x64/block_of_code.h"
#include "backend_x64/hostloc.h"
#include "backend_x64/oparg.h"
#include "common/common_types.h"
#include "frontend/ir/microinstruction.h"
#include "frontend/ir/value.h"

namespace Dynarmic {
namespace BackendX64 {

class RegAlloc;

struct HostLocInfo {
public:
    bool IsLocked() const {
        return is_being_used;
    }
    bool IsEmpty() const {
        return !is_being_used && values.empty();
    }
    bool IsLastUse() const {
        return !is_being_used && std::all_of(values.begin(), values.end(), [](const auto& inst) { return !inst->HasUses(); });
    }

    bool ContainsValue(const IR::Inst* inst) const {
        return std::find(values.begin(), values.end(), inst) != values.end();
    }

    void ReadLock() {
        ASSERT(!is_scratch);
        is_being_used = true;
    }
    void WriteLock() {
        ASSERT(!is_being_used);
        is_being_used = true;
        is_scratch = true;
    }
    void AddValue(IR::Inst* inst) {
        values.push_back(inst);
    }

    void EndOfAllocScope() {
        const auto to_erase = std::remove_if(values.begin(), values.end(), [](const auto& inst){ return !inst->HasUses(); });
        values.erase(to_erase, values.end());

        is_being_used = false;
        is_scratch = false;
    }

private:
    std::vector<IR::Inst*> values;
    bool is_being_used = false;
    bool is_scratch = false;
};

struct Argument {
public:
    IR::Type GetType() const {
        return value.GetType();
    }
    bool IsImmediate() const {
        return value.IsImmediate();
    }

    bool GetImmediateU1() const;
    u8 GetImmediateU8() const;
    u16 GetImmediateU16() const;
    u32 GetImmediateU32() const;
    u64 GetImmediateU64() const;

    /// Is this value currently in a GPR?
    bool IsInGpr() const;
    /// Is this value currently in a XMM?
    bool IsInXmm() const;
    /// Is this value currently in memory?
    bool IsInMemory() const;

private:
    friend class RegAlloc;
    Argument(RegAlloc& reg_alloc) : reg_alloc(reg_alloc) {}

    bool allocated = false;
    RegAlloc& reg_alloc;
    IR::Value value;
};

class RegAlloc final {
public:
    explicit RegAlloc(BlockOfCode* code) : code(code) {}

    std::array<Argument, 3> GetArgumentInfo(IR::Inst* inst);

    Xbyak::Reg64 UseGpr(Argument& arg) {
        ASSERT(!arg.allocated);
        arg.allocated = true;
        return HostLocToReg64(UseHostLocReg(arg.value, any_gpr));
    }
    Xbyak::Xmm UseXmm(Argument& arg) {
        ASSERT(!arg.allocated);
        arg.allocated = true;
        return HostLocToXmm(UseHostLocReg(arg.value, any_xmm));
    }
    OpArg UseOpArg(Argument& arg) {
        return UseGpr(arg);
    }
    void Use(Argument& arg, HostLoc host_loc) {
        ASSERT(!arg.allocated);
        arg.allocated = true;
        UseHostLocReg(arg.value, {host_loc});
    }

    Xbyak::Reg64 UseScratchGpr(Argument& arg) {
        ASSERT(!arg.allocated);
        arg.allocated = true;
        return HostLocToReg64(UseScratchHostLocReg(arg.value, any_gpr));
    }
    Xbyak::Xmm UseScratchXmm(Argument& arg) {
        ASSERT(!arg.allocated);
        arg.allocated = true;
        return HostLocToXmm(UseScratchHostLocReg(arg.value, any_xmm));
    }
    void UseScratch(Argument& arg, HostLoc host_loc) {
        ASSERT(!arg.allocated);
        arg.allocated = true;
        UseScratchHostLocReg(arg.value, {host_loc});
    }

    void DefineValue(IR::Inst* inst, const Xbyak::Reg& reg) {
        ASSERT(reg.getKind() == Xbyak::Operand::XMM || reg.getKind() == Xbyak::Operand::REG);
        HostLoc hostloc = static_cast<HostLoc>(reg.getIdx() + static_cast<size_t>(reg.getKind() == Xbyak::Operand::XMM ? HostLoc::XMM0 : HostLoc::RAX));
        DefineValue(inst, hostloc);
    }
    void DefineValue(IR::Inst* inst, Argument& arg) {
        ASSERT(!arg.allocated);
        arg.allocated = true;
        RegisterAddDef(inst, arg.value);
    }

    /// Late-def
    Xbyak::Reg64 DefGpr(IR::Inst* def_inst, HostLocList desired_locations = any_gpr) {
        HostLoc location = ScratchHostLocReg(desired_locations);
        DefineValue(def_inst, location);
        return HostLocToReg64(location);
    }
    Xbyak::Xmm DefXmm(IR::Inst* def_inst, HostLocList desired_locations = any_xmm) {
        HostLoc location = ScratchHostLocReg(desired_locations);
        DefineValue(def_inst, location);
        return HostLocToXmm(location);
    }
    void RegisterAddDef(IR::Inst* def_inst, const IR::Value& use_inst);
    /// Early-use, Late-def
    Xbyak::Reg64 UseDefGpr(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_gpr) {
        HostLoc location = UseScratchHostLocReg(use_value, desired_locations);
        DefineValue(def_inst, location);
        return HostLocToReg64(location);
    }
    Xbyak::Xmm UseDefXmm(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_xmm) {
        HostLoc location = UseScratchHostLocReg(use_value, desired_locations);
        DefineValue(def_inst, location);
        return HostLocToXmm(location);
    }
    std::tuple<OpArg, Xbyak::Reg64> UseDefOpArgGpr(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_gpr) {
        OpArg op;
        HostLoc host_loc;
        std::tie(op, host_loc) = UseDefOpArgHostLocReg(use_value, def_inst, desired_locations);
        return std::make_tuple(op, HostLocToReg64(host_loc));
    }
    std::tuple<OpArg, Xbyak::Xmm> UseDefOpArgXmm(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_xmm) {
        OpArg op;
        HostLoc host_loc;
        std::tie(op, host_loc) = UseDefOpArgHostLocReg(use_value, def_inst, desired_locations);
        return std::make_tuple(op, HostLocToXmm(host_loc));
    }
    /// Early-use
    Xbyak::Reg64 UseGpr(IR::Value use_value, HostLocList desired_locations = any_gpr) {
        return HostLocToReg64(UseHostLocReg(use_value, desired_locations));
    }
    Xbyak::Xmm UseXmm(IR::Value use_value, HostLocList desired_locations = any_xmm) {
        return HostLocToXmm(UseHostLocReg(use_value, desired_locations));
    }
    OpArg UseOpArg(IR::Value use_value, HostLocList desired_locations);
    /// Early-use, Destroyed
    Xbyak::Reg64 UseScratchGpr(IR::Value use_value, HostLocList desired_locations = any_gpr) {
        return HostLocToReg64(UseScratchHostLocReg(use_value, desired_locations));
    }
    Xbyak::Xmm UseScratchXmm(IR::Value use_value, HostLocList desired_locations = any_xmm) {
        return HostLocToXmm(UseScratchHostLocReg(use_value, desired_locations));
    }
    /// Early-def, Late-use, single-use
    Xbyak::Reg64 ScratchGpr(HostLocList desired_locations = any_gpr) {
        return HostLocToReg64(ScratchHostLocReg(desired_locations));
    }
    Xbyak::Xmm ScratchXmm(HostLocList desired_locations = any_xmm) {
        return HostLocToXmm(ScratchHostLocReg(desired_locations));
    }

    /// Late-def for result register, Early-use for all arguments, Each value is placed into registers according to host ABI.
    void HostCall(IR::Inst* result_def = nullptr, IR::Value arg0_use = {}, IR::Value arg1_use = {}, IR::Value arg2_use = {}, IR::Value arg3_use = {});

    // TODO: Values in host flags

    void EndOfAllocScope();

    void AssertNoMoreUses();

    void Reset();

private:
    friend struct Argument;

    HostLoc SelectARegister(HostLocList desired_locations) const;
    boost::optional<HostLoc> ValueLocation(const IR::Inst* value) const;

    void DefineValue(IR::Inst* def_inst, HostLoc host_loc);

    std::tuple<OpArg, HostLoc> UseDefOpArgHostLocReg(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations);
    HostLoc UseHostLocReg(IR::Value use_value, HostLocList desired_locations);
    HostLoc UseHostLocReg(IR::Inst* use_inst, HostLocList desired_locations);
    HostLoc UseScratchHostLocReg(IR::Value use_value, HostLocList desired_locations);
    HostLoc UseScratchHostLocReg(IR::Inst* use_inst, HostLocList desired_locations);
    HostLoc ScratchHostLocReg(HostLocList desired_locations);

    HostLoc LoadImmediateIntoHostLocReg(IR::Value imm, HostLoc reg);

    void Move(HostLoc to, HostLoc from);
    void CopyToScratch(HostLoc to, HostLoc from);
    void Exchange(HostLoc a, HostLoc b);
    void MoveOutOfTheWay(HostLoc reg);

    void SpillRegister(HostLoc loc);
    HostLoc FindFreeSpill() const;

    BlockOfCode* code = nullptr;

    std::array<HostLocInfo, HostLocCount> hostloc_info;
    HostLocInfo& LocInfo(HostLoc loc) {
        DEBUG_ASSERT(loc != HostLoc::RSP && loc != HostLoc::R15);
        return hostloc_info[static_cast<size_t>(loc)];
    }
    const HostLocInfo& LocInfo(HostLoc loc) const {
        DEBUG_ASSERT(loc != HostLoc::RSP && loc != HostLoc::R15);
        return hostloc_info[static_cast<size_t>(loc)];
    }
};

} // namespace BackendX64
} // namespace Dynarmic
