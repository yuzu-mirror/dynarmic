/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <catch.hpp>
#include <dynarmic/A32/a32.h>

#include "A32/testenv.h"

using namespace Dynarmic;

static A32::UserConfig GetUserConfig(ArmTestEnv* testenv) {
    A32::UserConfig user_config;
    user_config.enable_fast_dispatch = false;
    user_config.callbacks = testenv;
    return user_config;
}

TEST_CASE("arm: Opt Failure: Const folding in MostSignificantWord", "[arm][A32]") {
    // This was a randomized test-case that was failing.
    // This was due to constant folding for MostSignificantWord
    // failing to take into account an associated GetCarryFromOp
    // pseudoinstruction.

    ArmTestEnv test_env;
    A32::Jit jit{GetUserConfig(&test_env)};
    test_env.code_mem = {
        0xe30ad071, // movw, sp, #41073
        0xe75efd3d, // smmulr lr, sp, sp
        0xa637af1e, // shadd16ge r10, r7, lr
        0xf57ff01f, // clrex
        0x86b98879, // sxtahhi r8, r9, r9, ror #16
        0xeafffffe, // b +#0
    };

    jit.SetCpsr(0x000001d0); // User-mode

    test_env.ticks_left = 6;
    jit.Run();

    // If we don't trigger the GetCarryFromOp ASSERT, we're fine.
}
