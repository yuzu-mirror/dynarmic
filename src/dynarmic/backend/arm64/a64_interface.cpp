/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <memory>
#include <mutex>

#include <boost/icl/interval_set.hpp>
#include <mcl/assert.hpp>
#include <mcl/scope_exit.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/backend/arm64/a64_address_space.h"
#include "dynarmic/backend/arm64/a64_core.h"
#include "dynarmic/backend/arm64/a64_jitstate.h"
#include "dynarmic/common/atomic.h"
#include "dynarmic/interface/A64/a64.h"
#include "dynarmic/interface/A64/config.h"

namespace Dynarmic::A64 {

using namespace Backend::Arm64;

struct Jit::Impl final {
    Impl(Jit* jit_interface, A64::UserConfig conf)
            : jit_interface(jit_interface)
            , conf(conf)
            , current_address_space(conf)
            , core(conf) {}

    HaltReason Run() {
        ASSERT(!is_executing);
        PerformRequestedCacheInvalidation();

        is_executing = true;
        SCOPE_EXIT {
            is_executing = false;
        };

        HaltReason hr = core.Run(current_address_space, current_state, &halt_reason);

        PerformRequestedCacheInvalidation();

        return hr;
    }

    HaltReason Step() {
        ASSERT(!is_executing);
        PerformRequestedCacheInvalidation();

        is_executing = true;
        SCOPE_EXIT {
            is_executing = false;
        };

        HaltReason hr = core.Step(current_address_space, current_state, &halt_reason);

        PerformRequestedCacheInvalidation();

        return hr;
    }

    void ClearCache() {
        std::unique_lock lock{invalidation_mutex};
        invalidate_entire_cache = true;
        HaltExecution(HaltReason::CacheInvalidation);
    }

    void InvalidateCacheRange(std::uint64_t start_address, std::size_t length) {
        std::unique_lock lock{invalidation_mutex};
        invalid_cache_ranges.add(boost::icl::discrete_interval<u64>::closed(start_address, start_address + length - 1));
        HaltExecution(HaltReason::CacheInvalidation);
    }

    void Reset() {
        current_state = {};
    }

    void HaltExecution(HaltReason hr) {
        Atomic::Or(&halt_reason, static_cast<u32>(hr));
    }

    void ClearHalt(HaltReason hr) {
        Atomic::And(&halt_reason, ~static_cast<u32>(hr));
    }

    std::array<std::uint64_t, 31>& Regs() {
        return current_state.reg;
    }

    const std::array<std::uint64_t, 31>& Regs() const {
        return current_state.reg;
    }

    std::array<std::uint64_t, 64>& VecRegs() {
        return current_state.vec;
    }

    const std::array<std::uint64_t, 64>& VecRegs() const {
        return current_state.vec;
    }

    std::uint32_t Fpcr() const {
        return current_state.fpcr;
    }

    void SetFpcr(std::uint32_t value) {
        current_state.fpcr = value;
    }

    std::uint32_t Fpsr() const {
        return current_state.fpsr;
    }

    void SetFpscr(std::uint32_t value) {
        current_state.fpsr = value;
    }

    std::uint32_t Pstate() const {
        return current_state.cpsr_nzcv;
    }

    void SetPstate(std::uint32_t value) {
        current_state.cpsr_nzcv = value;
    }

    void ClearExclusiveState() {
        current_state.exclusive_state = false;
    }

    void DumpDisassembly() const {
        ASSERT_FALSE("Unimplemented");
    }

private:
    void PerformRequestedCacheInvalidation() {
        ClearHalt(HaltReason::CacheInvalidation);

        if (invalidate_entire_cache) {
            current_address_space.ClearCache();

            invalidate_entire_cache = false;
            invalid_cache_ranges.clear();
            return;
        }

        if (!invalid_cache_ranges.empty()) {
            // TODO: Optimize
            current_address_space.ClearCache();

            invalid_cache_ranges.clear();
            return;
        }
    }

    Jit* jit_interface;
    A64::UserConfig conf;
    A64JitState current_state{};
    A64AddressSpace current_address_space;
    A64Core core;

    volatile u32 halt_reason = 0;

    std::mutex invalidation_mutex;
    boost::icl::interval_set<u64> invalid_cache_ranges;
    bool invalidate_entire_cache = false;
    bool is_executing = false;
};

Jit::Jit(UserConfig conf) {
    (void)conf;
}

Jit::~Jit() = default;

HaltReason Jit::Run() {
    ASSERT_FALSE("not implemented");
}

HaltReason Jit::Step() {
    ASSERT_FALSE("not implemented");
}

void Jit::ClearCache() {
}

void Jit::InvalidateCacheRange(std::uint64_t start_address, std::size_t length) {
    (void)start_address;
    (void)length;
}

void Jit::Reset() {
}

void Jit::HaltExecution(HaltReason hr) {
    (void)hr;
}

void Jit::ClearHalt(HaltReason hr) {
    (void)hr;
}

std::uint64_t Jit::GetSP() const {
    return 0;
}

void Jit::SetSP(std::uint64_t value) {
    (void)value;
}

std::uint64_t Jit::GetPC() const {
    return 0;
}

void Jit::SetPC(std::uint64_t value) {
    (void)value;
}

std::uint64_t Jit::GetRegister(std::size_t index) const {
    (void)index;
    return 0;
}

void Jit::SetRegister(size_t index, std::uint64_t value) {
    (void)index;
    (void)value;
}

std::array<std::uint64_t, 31> Jit::GetRegisters() const {
    return {};
}

void Jit::SetRegisters(const std::array<std::uint64_t, 31>& value) {
    (void)value;
}

Vector Jit::GetVector(std::size_t index) const {
    (void)index;
    return {};
}

void Jit::SetVector(std::size_t index, Vector value) {
    (void)index;
    (void)value;
}

std::array<Vector, 32> Jit::GetVectors() const {
    return {};
}

void Jit::SetVectors(const std::array<Vector, 32>& value) {
    (void)value;
}

std::uint32_t Jit::GetFpcr() const {
    return 0;
}

void Jit::SetFpcr(std::uint32_t value) {
    (void)value;
}

std::uint32_t Jit::GetFpsr() const {
    return 0;
}

void Jit::SetFpsr(std::uint32_t value) {
    (void)value;
}

std::uint32_t Jit::GetPstate() const {
    return 0;
}

void Jit::SetPstate(std::uint32_t value) {
    (void)value;
}

void Jit::ClearExclusiveState() {
}

bool Jit::IsExecuting() const {
    return false;
}

void Jit::DumpDisassembly() const {
    ASSERT_FALSE("not implemented");
}

std::vector<std::string> Jit::Disassemble() const {
    ASSERT_FALSE("not implemented");
}

}  // namespace Dynarmic::A64
