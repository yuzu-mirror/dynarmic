/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <map>
#include <vector>

#include <boost/optional.hpp>
#include <xbyak.h>

#include "backend_x64/block_of_code.h"
#include "backend_x64/hostloc.h"
#include "backend_x64/jitstate.h"
#include "common/common_types.h"
#include "frontend/ir/microinstruction.h"
#include "frontend/ir/value.h"

namespace Dynarmic {
namespace BackendX64 {

struct OpArg {
    OpArg() : type(OPERAND), inner_operand() {}
    OpArg(const Xbyak::Address& address) : type(ADDRESS), inner_address(address) {}
    OpArg(const Xbyak::Operand& operand) : type(OPERAND), inner_operand(operand) {}

    Xbyak::Operand& operator*() {
        switch (type) {
        case ADDRESS:
            return inner_address;
        case OPERAND:
            return inner_operand;
        }
        ASSERT_MSG(false, "Unreachable");
    }

    void setBit(int bits) {
        switch (type) {
        case ADDRESS:
            inner_address.setBit(bits);
            return;
        case OPERAND:
            inner_operand.setBit(bits);
            return;
        }
        ASSERT_MSG(false, "Unreachable");
    }

private:
    enum {
        OPERAND,
        ADDRESS,
    } type;

    union {
        Xbyak::Operand inner_operand;
        Xbyak::Address inner_address;
    };
};

class RegAlloc final {
public:
    RegAlloc(BlockOfCode* code) : code(code) {}

    /// Late-def
    Xbyak::Reg64 DefGpr(IR::Inst* def_inst, HostLocList desired_locations = any_gpr) {
        return HostLocToReg64(DefHostLocReg(def_inst, desired_locations));
    }
    Xbyak::Xmm DefXmm(IR::Inst* def_inst, HostLocList desired_locations = any_xmm) {
        return HostLocToXmm(DefHostLocReg(def_inst, desired_locations));
    }
    void RegisterAddDef(IR::Inst* def_inst, const IR::Value& use_inst);
    /// Early-use, Late-def
    Xbyak::Reg64 UseDefGpr(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_gpr) {
        return HostLocToReg64(UseDefHostLocReg(use_value, def_inst, desired_locations));
    }
    Xbyak::Xmm UseDefXmm(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_xmm) {
        return HostLocToXmm(UseDefHostLocReg(use_value, def_inst, desired_locations));
    }
    std::tuple<OpArg, Xbyak::Reg64> UseDefOpArgGpr(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_gpr) {
        OpArg op;
        HostLoc host_loc;
        std::tie(op, host_loc) = UseDefOpArgHostLocReg(use_value, def_inst, desired_locations);
        return std::make_tuple(op, HostLocToReg64(host_loc));
    }
    std::tuple<OpArg, Xbyak::Xmm> UseDefOpArgXmm(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_gpr) {
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

    void DecrementRemainingUses(IR::Inst* value);

    void EndOfAllocScope();

    void AssertNoMoreUses();

    void Reset();

private:
    HostLoc SelectARegister(HostLocList desired_locations) const;
    boost::optional<HostLoc> ValueLocation(const IR::Inst* value) const;
    bool IsRegisterOccupied(HostLoc loc) const;
    bool IsRegisterAllocated(HostLoc loc) const;
    bool IsLastUse(IR::Inst* inst) const;

    HostLoc DefHostLocReg(IR::Inst* def_inst, HostLocList desired_locations);
    HostLoc UseDefHostLocReg(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations);
    HostLoc UseDefHostLocReg(IR::Inst* use_inst, IR::Inst* def_inst, HostLocList desired_locations);
    std::tuple<OpArg, HostLoc> UseDefOpArgHostLocReg(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations);
    HostLoc UseHostLocReg(IR::Value use_value, HostLocList desired_locations);
    HostLoc UseHostLocReg(IR::Inst* use_inst, HostLocList desired_locations);
    std::tuple<HostLoc, bool> UseHostLoc(IR::Inst* use_inst, HostLocList desired_locations);
    HostLoc UseScratchHostLocReg(IR::Value use_value, HostLocList desired_locations);
    HostLoc UseScratchHostLocReg(IR::Inst* use_inst, HostLocList desired_locations);
    HostLoc ScratchHostLocReg(HostLocList desired_locations);

    void EmitMove(HostLoc to, HostLoc from);
    void EmitExchange(HostLoc a, HostLoc b);
    HostLoc LoadImmediateIntoHostLocReg(IR::Value imm, HostLoc reg);

    void SpillRegister(HostLoc loc);
    HostLoc FindFreeSpill() const;

    BlockOfCode* code = nullptr;

    struct HostLocInfo {
        std::vector<IR::Inst*> values; // early value
        IR::Inst* def = nullptr; // late value
        bool is_being_used = false;

        bool IsIdle() const {
            return !is_being_used;
        }
        bool IsScratch() const {
            return is_being_used && !def && values.empty();
        }
        bool IsUse() const {
            return is_being_used && !def && !values.empty();
        }
        bool IsDef() const {
            return is_being_used && def && values.empty();
        }
        bool IsUseDef() const {
            return is_being_used && def && !values.empty();
        }
    };
    std::array<HostLocInfo, HostLocCount> hostloc_info;
    HostLocInfo& LocInfo(HostLoc loc) {
        return hostloc_info[static_cast<size_t>(loc)];
    }
    const HostLocInfo& LocInfo(HostLoc loc) const {
        return hostloc_info[static_cast<size_t>(loc)];
    }
};

} // namespace BackendX64
} // namespace Dynarmic
