/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <catch.hpp>

#include "backend_x64/emit_x64.h"
#include "common/common_types.h"
#include "frontend_arm/decoder/thumb1.h"
#include "frontend_arm/translate_thumb.h"

struct TinyBlockOfCode : Gen::XCodeBlock {
    TinyBlockOfCode() {
        AllocCodeSpace(256);
    }
};

void RunSingleThumbInstruction(u16 thumb_instruction, Dynarmic::BackendX64::JitState* jit_state_ptr) {
    Dynarmic::Arm::TranslatorVisitor visitor;
    auto decoder = Dynarmic::Arm::DecodeThumb1<Dynarmic::Arm::TranslatorVisitor>(thumb_instruction);
    REQUIRE(!!decoder);
    decoder->call(visitor, thumb_instruction);

    TinyBlockOfCode block_of_code;
    Dynarmic::BackendX64::Routines routines;
    Dynarmic::UserCallbacks callbacks;
    Dynarmic::BackendX64::EmitX64 emitter(&block_of_code, &routines, callbacks);

    Dynarmic::BackendX64::CodePtr code = emitter.Emit(visitor.ir.block);
    routines.RunCode(jit_state_ptr, code, 1);
}

TEST_CASE( "thumb: lsls r0, r1, #2", "[thumb]" ) {
    Dynarmic::BackendX64::JitState jit_state;
    jit_state.Reg[0] = 1;
    jit_state.Reg[1] = 2;
    jit_state.Cpsr = 0;

    RunSingleThumbInstruction(0x0088, &jit_state);

    REQUIRE( jit_state.Reg[0] == 8 );
    REQUIRE( jit_state.Reg[1] == 2 );
    REQUIRE( jit_state.Cpsr == 0 );
}

TEST_CASE( "thumb: lsls r0, r1, #31", "[thumb]" ) {
    Dynarmic::BackendX64::JitState jit_state;
    jit_state.Reg[0] = 1;
    jit_state.Reg[1] = 0xFFFFFFFF;
    jit_state.Cpsr = 0;

    RunSingleThumbInstruction(0x07C8, &jit_state);

    REQUIRE( jit_state.Reg[0] == 0x80000000 );
    REQUIRE( jit_state.Reg[1] == 0xffffffff );
    REQUIRE( jit_state.Cpsr == 0x20000000 );
}
