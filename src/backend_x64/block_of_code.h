/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>

#include "backend_x64/jitstate.h"
#include "common/common_types.h"
#include "common/x64/emitter.h"

namespace Dynarmic {
namespace BackendX64 {

class BlockOfCode final : public Gen::XCodeBlock {
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

    Gen::OpArg MFloatPositiveZero32() const {
        return Gen::M(const_FloatPositiveZero32);
    }
    Gen::OpArg MFloatNegativeZero32() const {
        return Gen::M(const_FloatNegativeZero32);
    }
    Gen::OpArg MFloatNaN32() const {
        return Gen::M(const_FloatNaN32);
    }
    Gen::OpArg MFloatNonSignMask32() const {
        return Gen::M(const_FloatNonSignMask32);
    }
    Gen::OpArg MFloatPositiveZero64() const {
        return Gen::M(const_FloatPositiveZero64);
    }
    Gen::OpArg MFloatNegativeZero64() const {
        return Gen::M(const_FloatNegativeZero64);
    }
    Gen::OpArg MFloatNaN64() const {
        return Gen::M(const_FloatNaN64);
    }
    Gen::OpArg MFloatNonSignMask64() const {
        return Gen::M(const_FloatNonSignMask64);
    }
    Gen::OpArg MFloatPenultimatePositiveDenormal64() const {
        return Gen::M(const_FloatPenultimatePositiveDenormal64);
    }
    Gen::OpArg MFloatMinS32() const {
        return Gen::M(const_FloatMinS32);
    }
    Gen::OpArg MFloatMaxS32() const {
        return Gen::M(const_FloatMaxS32);
    }
    Gen::OpArg MFloatMinU32() const {
        return Gen::M(const_FloatMinU32);
    }
    Gen::OpArg MFloatMaxU32() const {
        return Gen::M(const_FloatMaxU32);
    }

    CodePtr GetReturnFromRunCodeAddress() const {
        return return_from_run_code;
    }

private:
    const u8* const_FloatPositiveZero32 = nullptr;
    const u8* const_FloatNegativeZero32 = nullptr;
    const u8* const_FloatNaN32 = nullptr;
    const u8* const_FloatNonSignMask32 = nullptr;
    const u8* const_FloatPositiveZero64 = nullptr;
    const u8* const_FloatNegativeZero64 = nullptr;
    const u8* const_FloatNaN64 = nullptr;
    const u8* const_FloatNonSignMask64 = nullptr;
    const u8* const_FloatPenultimatePositiveDenormal64 = nullptr;
    const u8* const_FloatMinS32 = nullptr;
    const u8* const_FloatMaxS32 = nullptr;
    const u8* const_FloatMinU32 = nullptr;
    const u8* const_FloatMaxU32 = nullptr;
    void GenConstants();

    using RunCodeFuncType = void(*)(JitState*, CodePtr);
    RunCodeFuncType run_code = nullptr;
    void GenRunCode();

    CodePtr return_from_run_code = nullptr;
    CodePtr return_from_run_code_without_mxcsr_switch = nullptr;
    void GenReturnFromRunCode();
};

} // namespace BackendX64
} // namespace Dynarmic
