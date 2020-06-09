/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <type_traits>

#include <xbyak.h>
#include <xbyak_util.h>

#include "backend/x64/callback.h"
#include "backend/x64/constant_pool.h"
#include "backend/x64/jitstate_info.h"
#include "common/cast_util.h"
#include "common/common_types.h"

namespace Dynarmic::Backend::X64 {

using CodePtr = const void*;

struct RunCodeCallbacks {
    std::unique_ptr<Callback> LookupBlock;
    std::unique_ptr<Callback> AddTicks;
    std::unique_ptr<Callback> GetTicksRemaining;
};

class BlockOfCode final : public Xbyak::CodeGenerator {
public:
    BlockOfCode(RunCodeCallbacks cb, JitStateInfo jsi, std::function<void(BlockOfCode&)> rcp);
    BlockOfCode(const BlockOfCode&) = delete;

    /// Call when external emitters have finished emitting their preludes.
    void PreludeComplete();

    /// Change permissions to RW. This is required to support systems with W^X enforced.
    void EnableWriting();
    /// Change permissions to RX. This is required to support systems with W^X enforced.
    void DisableWriting();

    /// Clears this block of code and resets code pointer to beginning.
    void ClearCache();
    /// Calculates how much space is remaining to use. This is the minimum of near code and far code.
    size_t SpaceRemaining() const;

    /// Runs emulated code from code_ptr.
    void RunCode(void* jit_state, CodePtr code_ptr) const;
    /// Runs emulated code from code_ptr for a single cycle.
    void StepCode(void* jit_state, CodePtr code_ptr) const;
    /// Code emitter: Returns to dispatcher
    void ReturnFromRunCode(bool mxcsr_already_exited = false);
    /// Code emitter: Returns to dispatcher, forces return to host
    void ForceReturnFromRunCode(bool mxcsr_already_exited = false);
    /// Code emitter: Makes guest MXCSR the current MXCSR
    void SwitchMxcsrOnEntry();
    /// Code emitter: Makes saved host MXCSR the current MXCSR
    void SwitchMxcsrOnExit();
    /// Code emitter: Updates cycles remaining my calling cb.AddTicks and cb.GetTicksRemaining
    /// @note this clobbers ABI caller-save registers
    void UpdateTicks();
    /// Code emitter: Performs a block lookup based on current state
    /// @note this clobbers ABI caller-save registers
    void LookupBlock();

    /// Code emitter: Calls the function
    template <typename FunctionPointer>
    void CallFunction(FunctionPointer fn) {
        static_assert(std::is_pointer_v<FunctionPointer> && std::is_function_v<std::remove_pointer_t<FunctionPointer>>,
                      "Supplied type must be a pointer to a function");

        const u64 address  = reinterpret_cast<u64>(fn);
        const u64 distance = address - (getCurr<u64>() + 5);

        if (distance >= 0x0000000080000000ULL && distance < 0xFFFFFFFF80000000ULL) {
            // Far call
            mov(rax, address);
            call(rax);
        } else {
            call(fn);
        }
    }

    /// Code emitter: Calls the lambda. Lambda must not have any captures.
    template <typename Lambda>
    void CallLambda(Lambda l) {
        CallFunction(Common::FptrCast(l));
    }

    Xbyak::Address MConst(const Xbyak::AddressFrame& frame, u64 lower, u64 upper = 0);

    /// Far code sits far away from the near code. Execution remains primarily in near code.
    /// "Cold" / Rarely executed instructions sit in far code, so the CPU doesn't fetch them unless necessary.
    void SwitchToFarCode();
    void SwitchToNearCode();

    CodePtr GetCodeBegin() const;
    size_t GetTotalCodeSize() const;

    const void* GetReturnFromRunCodeAddress() const {
        return return_from_run_code[0];
    }

    const void* GetForceReturnFromRunCodeAddress() const {
        return return_from_run_code[FORCE_RETURN];
    }

    void int3() { db(0xCC); }

    /// Allocate memory of `size` bytes from the same block of memory the code is in.
    /// This is useful for objects that need to be placed close to or within code.
    /// The lifetime of this memory is the same as the code around it.
    void* AllocateFromCodeSpace(size_t size);

    void SetCodePtr(CodePtr code_ptr);
    void EnsurePatchLocationSize(CodePtr begin, size_t size);

    // ABI registers
#ifdef _WIN32
    static const Xbyak::Reg64 ABI_RETURN;
    static const Xbyak::Reg64 ABI_PARAM1;
    static const Xbyak::Reg64 ABI_PARAM2;
    static const Xbyak::Reg64 ABI_PARAM3;
    static const Xbyak::Reg64 ABI_PARAM4;
    static const std::array<Xbyak::Reg64, 4> ABI_PARAMS;
#else
    static const Xbyak::Reg64 ABI_RETURN;
    static const Xbyak::Reg64 ABI_RETURN2;
    static const Xbyak::Reg64 ABI_PARAM1;
    static const Xbyak::Reg64 ABI_PARAM2;
    static const Xbyak::Reg64 ABI_PARAM3;
    static const Xbyak::Reg64 ABI_PARAM4;
    static const Xbyak::Reg64 ABI_PARAM5;
    static const Xbyak::Reg64 ABI_PARAM6;
    static const std::array<Xbyak::Reg64, 6> ABI_PARAMS;
#endif

    JitStateInfo GetJitStateInfo() const { return jsi; }

    bool HasSSSE3() const;
    bool HasSSE41() const;
    bool HasSSE42() const;
    bool HasPCLMULQDQ() const;
    bool HasAVX() const;
    bool HasF16C() const;
    bool HasAESNI() const;
    bool HasLZCNT() const;
    bool HasBMI1() const;
    bool HasBMI2() const;
    bool HasFastBMI2() const;
    bool HasFMA() const;
    bool HasAVX2() const;
    bool HasAVX512_Skylake() const;
    bool HasAVX512_BITALG() const;

private:
    RunCodeCallbacks cb;
    JitStateInfo jsi;

    bool prelude_complete = false;
    CodePtr near_code_begin;
    CodePtr far_code_begin;

    ConstantPool constant_pool;

    bool in_far_code = false;
    CodePtr near_code_ptr;
    CodePtr far_code_ptr;

    using RunCodeFuncType = void(*)(void*, CodePtr);
    RunCodeFuncType run_code = nullptr;
    RunCodeFuncType step_code = nullptr;
    static constexpr size_t MXCSR_ALREADY_EXITED = 1 << 0;
    static constexpr size_t FORCE_RETURN = 1 << 1;
    std::array<const void*, 4> return_from_run_code;
    void GenRunCode(std::function<void(BlockOfCode&)> rcp);

    Xbyak::util::Cpu cpu_info;
    bool DoesCpuSupport(Xbyak::util::Cpu::Type type) const;
};

} // namespace Dynarmic::Backend::X64
