/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include <biscuit/assembler.hpp>
#include <biscuit/registers.hpp>
#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>
#include <mcl/type_traits/is_instance_of_template.hpp>
#include <tsl/robin_set.h>

#include "dynarmic/backend/riscv64/stack_layout.h"
#include "dynarmic/ir/cond.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::Backend::RV64 {

class RegAlloc;

struct HostLoc {
    enum class Kind {
        Gpr,
        Fpr,
        Spill,
    } kind;
    u32 index;
};

struct Argument {
public:
    using copyable_reference = std::reference_wrapper<Argument>;

    IR::Type GetType() const;
    bool IsImmediate() const;

    bool GetImmediateU1() const;
    u8 GetImmediateU8() const;
    u16 GetImmediateU16() const;
    u32 GetImmediateU32() const;
    u64 GetImmediateU64() const;
    IR::Cond GetImmediateCond() const;
    IR::AccType GetImmediateAccType() const;

private:
    friend class RegAlloc;
    explicit Argument(RegAlloc& reg_alloc)
            : reg_alloc{reg_alloc} {}

    bool allocated = false;
    RegAlloc& reg_alloc;
    IR::Value value;
};

template<typename T>
struct RAReg {
public:
    static constexpr bool is_fpr = std::is_base_of_v<biscuit::FPR, T>;

    operator T() const { return *reg; }

    T operator*() const { return *reg; }

    ~RAReg();

private:
    friend class RegAlloc;
    explicit RAReg(RegAlloc& reg_alloc, bool write, const IR::Inst* value)
            : reg_alloc{reg_alloc}, write{write}, value{value} {}

    void Realize();

    RegAlloc& reg_alloc;
    bool write;
    const IR::Inst* value;
    std::optional<T> reg;
};

struct HostLocInfo final {
    std::vector<const IR::Inst*> values;
    bool locked = false;
    bool realized = false;
    size_t accumulated_uses = 0;
    size_t expected_uses = 0;

    bool Contains(const IR::Inst*) const;
};

class RegAlloc {
public:
    using ArgumentInfo = std::array<Argument, IR::max_arg_count>;

    explicit RegAlloc(biscuit::Assembler& as, std::vector<u32> gpr_order, std::vector<u32> fpr_order)
            : as{as}, gpr_order{gpr_order}, fpr_order{fpr_order}, rand_gen{std::random_device{}()} {}

    ArgumentInfo GetArgumentInfo(IR::Inst* inst);
    bool IsValueLive(IR::Inst* inst) const;

    auto ReadX(Argument& arg) { return RAReg<biscuit::GPR>{*this, false, PreReadImpl(arg.value)}; }
    auto ReadD(Argument& arg) { return RAReg<biscuit::FPR>{*this, false, PreReadImpl(arg.value)}; }

    auto WriteX(IR::Inst* inst) { return RAReg<biscuit::GPR>{*this, true, inst}; }
    auto WriteD(IR::Inst* inst) { return RAReg<biscuit::FPR>{*this, true, inst}; }

    void SpillAll();

    template<typename... Ts>
    static void Realize(Ts&... rs) {
        static_assert((mcl::is_instance_of_template<RAReg, Ts>() && ...));
        (rs.Realize(), ...);
    }

private:
    template<typename>
    friend struct RAReg;

    const IR::Inst* PreReadImpl(const IR::Value& value) {
        ValueInfo(value.GetInst()).locked = true;
        return value.GetInst();
    }

    template<bool is_fpr>
    u32 RealizeReadImpl(const IR::Inst* value);
    template<bool is_fpr>
    u32 RealizeWriteImpl(const IR::Inst* value);
    void Unlock(HostLoc host_loc);

    u32 AllocateRegister(const std::array<HostLocInfo, 32>& regs, const std::vector<u32>& order) const;
    void SpillGpr(u32 index);
    void SpillFpr(u32 index);
    u32 FindFreeSpill() const;

    std::optional<HostLoc> ValueLocation(const IR::Inst* value) const;
    HostLocInfo& ValueInfo(HostLoc host_loc);
    HostLocInfo& ValueInfo(const IR::Inst* value);

    biscuit::Assembler& as;
    std::vector<u32> gpr_order;
    std::vector<u32> fpr_order;

    std::array<HostLocInfo, 32> gprs;
    std::array<HostLocInfo, 32> fprs;
    std::array<HostLocInfo, SpillCount> spills;

    mutable std::mt19937 rand_gen;
};

template<typename T>
RAReg<T>::~RAReg() {
    if (reg) {
        reg_alloc.Unlock(HostLoc{is_fpr ? HostLoc::Kind::Fpr : HostLoc::Kind::Gpr, reg->Index()});
    }
}

template<typename T>
void RAReg<T>::Realize() {
    reg = T{write ? reg_alloc.RealizeWriteImpl<is_fpr>(value) : reg_alloc.RealizeReadImpl<is_fpr>(value)};
}

}  // namespace Dynarmic::Backend::RV64
