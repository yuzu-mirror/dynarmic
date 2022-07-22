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
        Flags,
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

struct FlagsTag {
private:
    template<typename>
    friend struct RAReg;

    explicit FlagsTag(int) {}
    int index() const { return 0; }
};

template<typename T>
struct RAReg {
public:
    static constexpr HostLoc::Kind kind = !std::is_same_v<FlagsTag, T>
                                            ? std::is_base_of_v<oaknut::VReg, T>
                                                ? HostLoc::Kind::Fpr
                                                : HostLoc::Kind::Gpr
                                            : HostLoc::Kind::Flags;

    operator T() const { return *reg; }

    operator oaknut::WRegWsp() const requires(std::is_same_v<T, oaknut::WReg>) {
        return *reg;
    }

    operator oaknut::XRegSp() const requires(std::is_same_v<T, oaknut::XReg>) {
        return *reg;
    }

    T operator*() const { return *reg; }
    const T* operator->() const { return &*reg; }

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

    auto ReadFlags(Argument& arg) { return RAReg<FlagsTag>{*this, false, PreReadImpl(arg.value)}; }

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

    auto WriteFlags(IR::Inst* inst) { return RAReg<FlagsTag>{*this, true, inst}; }

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

    void DefineAsExisting(IR::Inst* inst, Argument& arg);

    void SpillFlags();
    void SpillAll();

    template<typename... Ts>
    static void Realize(Ts&... rs) {
        static_assert((mcl::is_instance_of_template<RAReg, Ts>() && ...));
        (rs.Realize(), ...);
    }

    void AssertNoMoreUses() const;

private:
    template<typename>
    friend struct RAReg;

    const IR::Inst* PreReadImpl(const IR::Value& value) {
        const IR::Inst* inst = value.GetInst();
        ValueInfo(inst).locked = true;
        return inst;
    }

    template<HostLoc::Kind kind>
    int RealizeReadImpl(const IR::Inst* value);
    template<HostLoc::Kind kind>
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
    HostLocInfo flags;
    std::array<HostLocInfo, SpillCount> spills;

    mutable std::mt19937 rand_gen;
};

template<typename T>
RAReg<T>::~RAReg() {
    if (reg) {
        reg_alloc.Unlock(HostLoc{kind, reg->index()});
    }
}

template<typename T>
void RAReg<T>::Realize() {
    reg = T{write ? reg_alloc.RealizeWriteImpl<kind>(value) : reg_alloc.RealizeReadImpl<kind>(value)};
}

}  // namespace Dynarmic::Backend::Arm64
