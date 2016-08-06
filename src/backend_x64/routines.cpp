/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <limits>

#include "backend_x64/jitstate.h"
#include "backend_x64/routines.h"
#include "common/x64/abi.h"

using namespace Gen;

namespace Dynarmic {
namespace BackendX64 {

Routines::Routines() {
    AllocCodeSpace(1024);

    GenConstants();
    GenRunCode();
}

size_t Routines::RunCode(JitState* jit_state, CodePtr basic_block, size_t cycles_to_run) const {
    constexpr size_t max_cycles_to_run = static_cast<size_t>(std::numeric_limits<decltype(jit_state->cycles_remaining)>::max());
    ASSERT(cycles_to_run <= max_cycles_to_run);

    jit_state->cycles_remaining = cycles_to_run;
    run_code(jit_state, basic_block);
    return cycles_to_run - jit_state->cycles_remaining; // Return number of cycles actually run.
}

void Routines::GenConstants() {
    const_FloatNegativeZero32 = AlignCode16();
    Write32(0x80000000u);
    const_FloatNaN32 = AlignCode16();
    Write32(0x7fc00000u);
    const_FloatNegativeZero64 = AlignCode16();
    Write64(0x8000000000000000u);
    const_FloatNaN64 = AlignCode16();
    Write64(0x7ff8000000000000u);
    const_FloatNonSignMask64 = AlignCode16();
    Write64(0x7fffffffffffffffu);
    const_FloatPenultimatePositiveDenormal64 = AlignCode16();
    Write64(0x000ffffffffffffeu);
    AlignCode16();
}

void Routines::GenRunCode() {
    run_code = reinterpret_cast<RunCodeFuncType>(const_cast<u8*>(this->GetCodePtr()));

    // This serves two purposes:
    // 1. It saves all the registers we as a callee need to save.
    // 2. It aligns the stack so that the code the JIT emits can assume
    //    that the stack is appropriately aligned for CALLs.
    ABI_PushRegistersAndAdjustStack(ABI_ALL_CALLEE_SAVED, 8);

    MOV(64, R(R15), R(ABI_PARAM1));
    MOV(64, MDisp(R15, offsetof(JitState, save_host_RSP)), R(RSP));
    STMXCSR(MDisp(R15, offsetof(JitState, save_host_MXCSR)));
    LDMXCSR(MDisp(R15, offsetof(JitState, guest_MXCSR)));

    JMPptr(R(ABI_PARAM2));
}

void Routines::GenReturnFromRunCode(XEmitter* code) const {
    code->STMXCSR(MDisp(R15, offsetof(JitState, guest_MXCSR)));
    code->LDMXCSR(MDisp(R15, offsetof(JitState, save_host_MXCSR)));
    code->MOV(64, R(RSP), MDisp(R15, offsetof(JitState, save_host_RSP)));

    code->ABI_PopRegistersAndAdjustStack(ABI_ALL_CALLEE_SAVED, 8);
    code->RET();
}

} // namespace BackendX64
} // namespace Dynarmic
