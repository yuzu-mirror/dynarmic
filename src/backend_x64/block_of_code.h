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

    Gen::OpArg MFloatNegativeZero32() const {
        return Gen::M(const_FloatNegativeZero32);
    }
    Gen::OpArg MFloatNaN32() const {
        return Gen::M(const_FloatNaN32);
    }
    Gen::OpArg MFloatNonSignMask32() const {
        return Gen::M(const_FloatNonSignMask32);
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

    CodePtr GetReturnFromRunCodeAddress() const {
        return return_from_run_code;
    }

private:
    const u8* const_FloatNegativeZero32;
    const u8* const_FloatNaN32;
    const u8* const_FloatNonSignMask32;
    const u8* const_FloatNegativeZero64;
    const u8* const_FloatNaN64;
    const u8* const_FloatNonSignMask64;
    const u8* const_FloatPenultimatePositiveDenormal64;
    void GenConstants();

    using RunCodeFuncType = void(*)(JitState*, CodePtr);
    RunCodeFuncType run_code;
    void GenRunCode();

    CodePtr return_from_run_code;
    CodePtr return_from_run_code_without_mxcsr_switch;
    void GenReturnFromRunCode();
};

} // namespace BackendX64
} // namespace Dynarmic
