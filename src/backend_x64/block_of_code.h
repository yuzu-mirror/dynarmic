/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <vector>

#include <xbyak.h>

#include "backend_x64/jitstate.h"
#include "common/common_types.h"

namespace Dynarmic {
namespace BackendX64 {

class BlockOfCode final : public Xbyak::CodeGenerator {
public:
    BlockOfCode();

    /// Clears this block of code and resets code pointer to beginning.
    void ClearCache(bool poison_memory);

    /// Runs emulated code for approximately `cycles_to_run` cycles.
    size_t RunCode(JitState* jit_state, CodePtr basic_block, size_t cycles_to_run) const;
    /// Code emitter: Returns to host
    void ReturnFromRunCode(bool MXCSR_switch = true);
    /// Code emitter: Makes guest MXCSR the current MXCSR
    void SwitchMxcsrOnEntry();
    /// Code emitter: Makes saved host MXCSR the current MXCSR
    void SwitchMxcsrOnExit();
    /// Code emitter: Calls the function
    void CallFunction(const void* fn);

    Xbyak::Address MFloatPositiveZero32() {
        return xword[rip + const_FloatPositiveZero32];
    }
    Xbyak::Address MFloatNegativeZero32() {
        return xword[rip + const_FloatNegativeZero32];
    }
    Xbyak::Address MFloatNaN32() {
        return xword[rip + const_FloatNaN32];
    }
    Xbyak::Address MFloatNonSignMask32() {
        return xword[rip + const_FloatNonSignMask32];
    }
    Xbyak::Address MFloatPositiveZero64() {
        return xword[rip + const_FloatPositiveZero64];
    }
    Xbyak::Address MFloatNegativeZero64() {
        return xword[rip + const_FloatNegativeZero64];
    }
    Xbyak::Address MFloatNaN64() {
        return xword[rip + const_FloatNaN64];
    }
    Xbyak::Address MFloatNonSignMask64() {
        return xword[rip + const_FloatNonSignMask64];
    }
    Xbyak::Address MFloatPenultimatePositiveDenormal64() {
        return xword[rip + const_FloatPenultimatePositiveDenormal64];
    }
    Xbyak::Address MFloatMinS32() {
        return xword[rip + const_FloatMinS32];
    }
    Xbyak::Address MFloatMaxS32() {
        return xword[rip + const_FloatMaxS32];
    }
    Xbyak::Address MFloatMinU32() {
        return xword[rip + const_FloatMinU32];
    }
    Xbyak::Address MFloatMaxU32() {
        return xword[rip + const_FloatMaxU32];
    }

    const void* GetReturnFromRunCodeAddress() const {
        return return_from_run_code;
    }

    void int3() { db(0xCC); }
    void nop(size_t size = 0) {
        for (size_t i = 0; i < size; i++) {
            db(0x90);
        }
    }

    void SetCodePtr(CodePtr ptr);
    void EnsurePatchLocationSize(CodePtr begin, size_t size);

#ifdef _WIN32
    Xbyak::Reg64 ABI_RETURN = rax;
    Xbyak::Reg64 ABI_PARAM1 = rcx;
    Xbyak::Reg64 ABI_PARAM2 = rdx;
    Xbyak::Reg64 ABI_PARAM3 = r8;
    Xbyak::Reg64 ABI_PARAM4 = r9;
#else
    Xbyak::Reg64 ABI_RETURN = rax;
    Xbyak::Reg64 ABI_PARAM1 = rdi;
    Xbyak::Reg64 ABI_PARAM2 = rsi;
    Xbyak::Reg64 ABI_PARAM3 = rdx;
    Xbyak::Reg64 ABI_PARAM4 = rcx;
#endif

private:
    Xbyak::Label const_FloatPositiveZero32;
    Xbyak::Label const_FloatNegativeZero32;
    Xbyak::Label const_FloatNaN32;
    Xbyak::Label const_FloatNonSignMask32;
    Xbyak::Label const_FloatPositiveZero64;
    Xbyak::Label const_FloatNegativeZero64;
    Xbyak::Label const_FloatNaN64;
    Xbyak::Label const_FloatNonSignMask64;
    Xbyak::Label const_FloatPenultimatePositiveDenormal64;
    Xbyak::Label const_FloatMinS32;
    Xbyak::Label const_FloatMaxS32;
    Xbyak::Label const_FloatMinU32;
    Xbyak::Label const_FloatMaxU32;
    void GenConstants();

    using RunCodeFuncType = void(*)(JitState*, CodePtr);
    RunCodeFuncType run_code = nullptr;
    void GenRunCode();

    const void* return_from_run_code = nullptr;
    const void* return_from_run_code_without_mxcsr_switch = nullptr;
    void GenReturnFromRunCode();
};

} // namespace BackendX64
} // namespace Dynarmic
