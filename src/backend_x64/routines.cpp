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

    GenRunCode();
}

size_t Routines::RunCode(JitState* jit_state, CodePtr basic_block, size_t cycles_to_run) const {
    constexpr size_t max_cycles_to_run = static_cast<size_t>(std::numeric_limits<decltype(jit_state->cycles_remaining)>::max());
    ASSERT(cycles_to_run <= max_cycles_to_run);

    jit_state->cycles_remaining = cycles_to_run;
    run_code(jit_state, basic_block);
    return cycles_to_run - jit_state->cycles_remaining; // Return number of cycles actually run.
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

    JMPptr(R(ABI_PARAM2));
}

void Routines::GenReturnFromRunCode(XEmitter* code) const {
    code->MOV(64, R(RSP), MDisp(R15, offsetof(JitState, save_host_RSP)));
    code->ABI_PopRegistersAndAdjustStack(ABI_ALL_CALLEE_SAVED, 8);
    code->RET();
}

} // namespace BackendX64
} // namespace Dynarmic
