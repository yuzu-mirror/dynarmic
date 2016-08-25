/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <limits>

#include <xbyak.h>

#include "backend_x64/abi.h"
#include "backend_x64/block_of_code.h"
#include "backend_x64/jitstate.h"
#include "common/assert.h"

namespace Dynarmic {
namespace BackendX64 {

BlockOfCode::BlockOfCode() : Xbyak::CodeGenerator(128 * 1024 * 1024) {
    ClearCache(false);
}

void BlockOfCode::ClearCache(bool poison_memory) {
    consts.~Consts();
    new (&consts) Consts();
    reset();
    GenConstants();
    GenRunCode();
    GenReturnFromRunCode();
}

size_t BlockOfCode::RunCode(JitState* jit_state, CodePtr basic_block, size_t cycles_to_run) const {
    constexpr size_t max_cycles_to_run = static_cast<size_t>(std::numeric_limits<decltype(jit_state->cycles_remaining)>::max());
    ASSERT(cycles_to_run <= max_cycles_to_run);

    jit_state->cycles_remaining = cycles_to_run;
    run_code(jit_state, basic_block);
    return cycles_to_run - jit_state->cycles_remaining; // Return number of cycles actually run.
}

void BlockOfCode::ReturnFromRunCode(bool MXCSR_switch) {
    jmp(MXCSR_switch ? return_from_run_code : return_from_run_code_without_mxcsr_switch);
}

void BlockOfCode::GenConstants() {
    align();
    L(consts.FloatNegativeZero32);
    dd(0x80000000u);

    align();
    L(consts.FloatNaN32);
    dd(0x7fc00000u);

    align();
    L(consts.FloatNonSignMask32);
    dq(0x7fffffffu);

    align();
    L(consts.FloatNegativeZero64);
    dq(0x8000000000000000u);

    align();
    L(consts.FloatNaN64);
    dq(0x7ff8000000000000u);

    align();
    L(consts.FloatNonSignMask64);
    dq(0x7fffffffffffffffu);

    align();
    L(consts.FloatPenultimatePositiveDenormal64);
    dq(0x000ffffffffffffeu);

    align();
    L(consts.FloatMinS32);
    dq(0xc1e0000000000000u); // -2147483648 as a double

    align();
    L(consts.FloatMaxS32);
    dq(0x41dfffffffc00000u); // 2147483647 as a double

    align();
    L(consts.FloatPositiveZero32);
    L(consts.FloatPositiveZero64);
    L(consts.FloatMinU32);
    dq(0x0000000000000000u); // 0 as a double

    align();
    L(consts.FloatMaxU32);
    dq(0x41efffffffe00000u); // 4294967295 as a double

    align();
}

void BlockOfCode::GenRunCode() {
    align();
    run_code = getCurr<RunCodeFuncType>();

    // This serves two purposes:
    // 1. It saves all the registers we as a callee need to save.
    // 2. It aligns the stack so that the code the JIT emits can assume
    //    that the stack is appropriately aligned for CALLs.
    ABI_PushCalleeSaveRegistersAndAdjustStack(this);

    mov(r15, ABI_PARAM1);
    SwitchMxcsrOnEntry();
    jmp(ABI_PARAM2);
}

void BlockOfCode::GenReturnFromRunCode() {
    return_from_run_code = getCurr<const void*>();

    SwitchMxcsrOnExit();

    return_from_run_code_without_mxcsr_switch = getCurr<const void*>();

    ABI_PopCalleeSaveRegistersAndAdjustStack(this);
    ret();
}

void BlockOfCode::SwitchMxcsrOnEntry() {
    stmxcsr(dword[r15 + offsetof(JitState, save_host_MXCSR)]);
    ldmxcsr(dword[r15 + offsetof(JitState, guest_MXCSR)]);
}

void BlockOfCode::SwitchMxcsrOnExit() {
    stmxcsr(dword[r15 + offsetof(JitState, guest_MXCSR)]);
    ldmxcsr(dword[r15 + offsetof(JitState, save_host_MXCSR)]);
}

void BlockOfCode::CallFunction(const void* fn) {
    u64 distance = u64(fn) - (getCurr<u64>() + 5);
    if (distance >= 0x0000000080000000ULL && distance < 0xFFFFFFFF80000000ULL) {
        // Far call
        mov(rax, u64(fn));
        call(rax);
    } else {
        call(fn);
    }
}

void BlockOfCode::SetCodePtr(CodePtr ptr) {
    // The "size" defines where top_, the insertion point, is.
    size_t required_size = reinterpret_cast<const u8*>(ptr) - getCode();
    setSize(required_size);
}

void BlockOfCode::EnsurePatchLocationSize(CodePtr begin, size_t size) {
    size_t current_size = getCurr<const u8*>() - reinterpret_cast<const u8*>(begin);
    ASSERT(current_size <= size);
    nop(size - current_size);
}

} // namespace BackendX64
} // namespace Dynarmic
