/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <catch.hpp>

#include "common/common_types.h"
#include "interface/interface.h"
#include "skyeye_interpreter/dyncom/arm_dyncom_interpreter.h"
#include "skyeye_interpreter/skyeye_common/armstate.h"

static std::array<u16, 1024> code_mem{};

static u32 MemoryRead32(u32 vaddr);
static void InterpreterFallback(u32 pc, Dynarmic::Jit* jit);
static Dynarmic::UserCallbacks GetUserCallbacks();

static u32 MemoryRead32(u32 vaddr) {
    if (vaddr < code_mem.size() * sizeof(u16)) {
        size_t index = vaddr / sizeof(u16);
        return code_mem[index] | (code_mem[index+1] << 16);
    }
    return vaddr;
}

static void InterpreterFallback(u32 pc, Dynarmic::Jit* jit) {
    ARMul_State interp_state{USER32MODE};
    interp_state.user_callbacks = GetUserCallbacks();
    interp_state.NumInstrsToExecute = 1;

    interp_state.Reg = jit->Regs();
    interp_state.Cpsr = jit->Cpsr();
    interp_state.Reg[15] = pc;

    InterpreterClearCache();
    InterpreterMainLoop(&interp_state);

    jit->Regs() = interp_state.Reg;
    jit->Cpsr() = interp_state.Cpsr;
}

static Dynarmic::UserCallbacks GetUserCallbacks() {
    Dynarmic::UserCallbacks user_callbacks{};
    user_callbacks.MemoryRead32 = &MemoryRead32;
    user_callbacks.InterpreterFallback = &InterpreterFallback;
    return user_callbacks;
}

TEST_CASE( "thumb: lsls r0, r1, #2", "[thumb]" ) {
    Dynarmic::Jit jit{GetUserCallbacks()};
    code_mem.fill({});
    code_mem[0] = 0x0088; // lsls r0, r1, #2
    code_mem[1] = 0xE7FE; // b +#0

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[15] = 0; // PC = 0
    jit.Cpsr() = 0x00000030; // Thumb, User-mode

    jit.Run(1);

    REQUIRE( jit.Regs()[0] == 8 );
    REQUIRE( jit.Regs()[1] == 2 );
    REQUIRE( jit.Regs()[15] == 2 );
    REQUIRE( jit.Cpsr() == 0x00000030 );
}

TEST_CASE( "thumb: lsls r0, r1, #31", "[thumb]" ) {
    Dynarmic::Jit jit{GetUserCallbacks()};
    code_mem.fill({});
    code_mem[0] = 0x07C8; // lsls r0, r1, #31
    code_mem[1] = 0xE7FE; // b +#0

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 0xFFFFFFFF;
    jit.Regs()[15] = 0; // PC = 0
    jit.Cpsr() = 0x00000030; // Thumb, User-mode

    jit.Run(1);

    REQUIRE( jit.Regs()[0] == 0x80000000 );
    REQUIRE( jit.Regs()[1] == 0xffffffff );
    REQUIRE( jit.Regs()[15] == 2 );
    REQUIRE( jit.Cpsr() == 0x20000030 ); // C flag, Thumb, User-mode
}
