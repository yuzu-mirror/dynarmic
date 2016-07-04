/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <catch.hpp>

#include "common/common_types.h"
#include "interface/interface.h"

std::array<u32, 1024> code_mem{};

u32 MemoryRead32(u32 vaddr) {
    if (vaddr < code_mem.size() * sizeof(u32)) {
        return code_mem[vaddr / sizeof(u32)];
    }
    return vaddr;
}

Dynarmic::UserCallbacks GetUserCallbacks() {
    Dynarmic::UserCallbacks user_callbacks{};
    user_callbacks.MemoryRead32 = &MemoryRead32;
    return user_callbacks;
}

TEST_CASE( "thumb: lsls r0, r1, #2", "[thumb]" ) {
    Dynarmic::Jit jit{GetUserCallbacks()};
    code_mem.fill({});
    code_mem[0] = 0x0088; // lsls r0, r1, #2
    code_mem[1] = 0xDE00; // udf #0

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 2;
    jit.Regs()[15] = 0; // PC = 0
    jit.Cpsr() = 0x00000030; // Thumb, User-mode

    jit.Run(1);

    REQUIRE( jit.Regs()[0] == 8 );
    REQUIRE( jit.Regs()[1] == 2 );
    REQUIRE( jit.Cpsr() == 0x00000030 );
}

TEST_CASE( "thumb: lsls r0, r1, #31", "[thumb]" ) {
    Dynarmic::Jit jit{GetUserCallbacks()};
    code_mem.fill({});
    code_mem[0] = 0x07C8; // lsls r0, r1, #31
    code_mem[1] = 0xDE00; // udf #0

    jit.Regs()[0] = 1;
    jit.Regs()[1] = 0xFFFFFFFF;
    jit.Regs()[15] = 0; // PC = 0
    jit.Cpsr() = 0x00000030; // Thumb, User-mode

    jit.Run(1);

    REQUIRE( jit.Regs()[0] == 0x80000000 );
    REQUIRE( jit.Regs()[1] == 0xffffffff );
    REQUIRE( jit.Cpsr() == 0x20000030 ); // C flag, Thumb, User-mode
}
