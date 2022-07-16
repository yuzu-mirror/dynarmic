/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>
#include <mcl/type_traits/is_instance_of_template.hpp>
#include <oaknut/oaknut.hpp>
#include <tsl/robin_set.h>

#include "dynarmic/backend/arm64/stack_layout.h"
#include "dynarmic/ir/cond.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::Backend::Arm64 {

class RegAlloc;

struct HostLoc {
    enum class Kind {
        Gpr,
        Fpr,
        Spill,
    } kind;
    int index;
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
    static constexpr bool is_vector = std::is_base_of_v<oaknut::VReg, T>;

    operator T() const { return *reg; }

    template<typename U = T, typename = std::enable_if_t<std::is_same_v<U, oaknut::WReg> && std::is_same_v<T, U>>>
    operator oaknut::WRegWsp() const {
        return *reg;
    }

    template<typename U = T, typename = std::enable_if_t<std::is_same_v<U, oaknut::XReg> && std::is_same_v<T, U>>>
    operator oaknut::XRegSp() const {
        return *reg;
    }

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

struct HostLocInfo {
    tsl::robin_set<const IR::Inst*> values;
    bool locked = false;
    bool realized = false;
    size_t accumulated_uses = 0;
    size_t expected_uses = 0;
};

class RegAlloc {
public:
    using ArgumentInfo = std::array<Argument, IR::max_arg_count>;

    explicit RegAlloc(oaknut::CodeGenerator& code, std::vector<int> gpr_order, std::vector<int> fpr_order)
            : code{code}, gpr_order{gpr_order}, fpr_order{fpr_order}, rand_gen{std::random_device{}()} {}

    ArgumentInfo GetArgumentInfo(IR::Inst* inst);
    bool IsValueLive(IR::Inst* inst) const;

    auto ReadX(Argument& arg) { return RAReg<oaknut::XReg>{*this, false, PreReadImpl(arg.value)}; }
    auto ReadW(Argument& arg) { return RAReg<oaknut::WReg>{*this, false, PreReadImpl(arg.value)}; }

    auto ReadQ(Argument& arg) { return RAReg<oaknut::QReg>{*this, false, PreReadImpl(arg.value)}; }
    auto ReadD(Argument& arg) { return RAReg<oaknut::DReg>{*this, false, PreReadImpl(arg.value)}; }
    auto ReadS(Argument& arg) { return RAReg<oaknut::SReg>{*this, false, PreReadImpl(arg.value)}; }
    auto ReadH(Argument& arg) { return RAReg<oaknut::HReg>{*this, false, PreReadImpl(arg.value)}; }
    auto ReadB(Argument& arg) { return RAReg<oaknut::BReg>{*this, false, PreReadImpl(arg.value)}; }

    template<size_t size>
    auto ReadReg(Argument& arg) {
        if constexpr (size == 64) {
            return ReadX(arg);
        } else if constexpr (size == 32) {
            return ReadW(arg);
        } else {
            ASSERT_FALSE("Invalid size to ReadReg {}", size);
        }
    }

    template<size_t size>
    auto ReadVec(Argument& arg) {
        if constexpr (size == 128) {
            return ReadQ(arg);
        } else if constexpr (size == 64) {
            return ReadD(arg);
        } else if constexpr (size == 32) {
            return ReadS(arg);
        } else if constexpr (size == 16) {
            return ReadH(arg);
        } else if constexpr (size == 8) {
            return ReadB(arg);
        } else {
            ASSERT_FALSE("Invalid size to ReadVec {}", size);
        }
    }

    auto WriteX(IR::Inst* inst) { return RAReg<oaknut::XReg>{*this, true, inst}; }
    auto WriteW(IR::Inst* inst) { return RAReg<oaknut::WReg>{*this, true, inst}; }

    auto WriteQ(IR::Inst* inst) { return RAReg<oaknut::QReg>{*this, true, inst}; }
    auto WriteD(IR::Inst* inst) { return RAReg<oaknut::DReg>{*this, true, inst}; }
    auto WriteS(IR::Inst* inst) { return RAReg<oaknut::SReg>{*this, true, inst}; }
    auto WriteH(IR::Inst* inst) { return RAReg<oaknut::HReg>{*this, true, inst}; }
    auto WriteB(IR::Inst* inst) { return RAReg<oaknut::BReg>{*this, true, inst}; }

    template<size_t size>
    auto WriteReg(IR::Inst* inst) {
        if constexpr (size == 64) {
            return WriteX(inst);
        } else if constexpr (size == 32) {
            return WriteW(inst);
        } else {
            ASSERT_FALSE("Invalid size to WriteReg {}", size);
        }
    }

    template<size_t size>
    auto WriteVec(IR::Inst* inst) {
        if constexpr (size == 128) {
            return WriteQ(inst);
        } else if constexpr (size == 64) {
            return WriteD(inst);
        } else if constexpr (size == 32) {
            return WriteS(inst);
        } else if constexpr (size == 16) {
            return WriteH(inst);
        } else if constexpr (size == 8) {
            return WriteB(inst);
        } else {
            ASSERT_FALSE("Invalid size to WriteVec {}", size);
        }
    }

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

    template<bool is_vector>
    int RealizeReadImpl(const IR::Inst* value);
    template<bool is_vector>
    int RealizeWriteImpl(const IR::Inst* value);
    void Unlock(HostLoc host_loc);

    int AllocateRegister(const std::array<HostLocInfo, 32>& regs, const std::vector<int>& order) const;
    void SpillGpr(int index);
    void SpillFpr(int index);
    int FindFreeSpill() const;

    std::optional<HostLoc> ValueLocation(const IR::Inst* value) const;
    HostLocInfo& ValueInfo(HostLoc host_loc);
    HostLocInfo& ValueInfo(const IR::Inst* value);

    oaknut::CodeGenerator& code;
    std::vector<int> gpr_order;
    std::vector<int> fpr_order;

    std::array<HostLocInfo, 32> gprs;
    std::array<HostLocInfo, 32> fprs;
    std::array<HostLocInfo, SpillCount> spills;

    mutable std::mt19937 rand_gen;
};

template<typename T>
RAReg<T>::~RAReg() {
    if (reg) {
        reg_alloc.Unlock(HostLoc{is_vector ? HostLoc::Kind::Fpr : HostLoc::Kind::Gpr, reg->index()});
    }
}

template<typename T>
void RAReg<T>::Realize() {
    reg = T{write ? reg_alloc.RealizeWriteImpl<is_vector>(value) : reg_alloc.RealizeReadImpl<is_vector>(value)};
}

}  // namespace Dynarmic::Backend::Arm64
