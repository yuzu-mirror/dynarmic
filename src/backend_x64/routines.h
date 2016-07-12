/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "backend_x64/jitstate.h"
#include "common/common_types.h"
#include "common/x64/emitter.h"

namespace Dynarmic {
namespace BackendX64 {

class Routines final : private Gen::XCodeBlock {
public:
    Routines();

    size_t RunCode(JitState* jit_state, CodePtr basic_block, size_t cycles_to_run) const;
    void GenReturnFromRunCode(Gen::XEmitter* code) const;

private:
    using RunCodeFuncType = void(*)(JitState*, CodePtr);
    RunCodeFuncType run_code;
    void GenRunCode();
};

} // namespace BackendX64
} // namespace Dynarmic
