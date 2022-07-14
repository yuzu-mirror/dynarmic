/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/arm64/emit_arm64.h"

#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/a32_jitstate.h"
#include "dynarmic/ir/basic_block.h"

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

EmittedBlockInfo EmitArm64(oaknut::CodeGenerator& code, IR::Block block) {
    (void)block;

    EmittedBlockInfo ebi;
    ebi.entry_point = code.ptr<CodePtr>();

    code.MOV(W0, 8);
    code.STR(W0, X1, offsetof(A32JitState, regs) + 0 * sizeof(u32));
    code.MOV(W0, 2);
    code.STR(W0, X1, offsetof(A32JitState, regs) + 1 * sizeof(u32));
    code.STR(W0, X1, offsetof(A32JitState, regs) + 15 * sizeof(u32));

    ebi.relocations[code.ptr<CodePtr>() - ebi.entry_point] = LinkTarget::ReturnFromRunCode;
    code.NOP();

    ebi.size = code.ptr<CodePtr>() - ebi.entry_point;
    return ebi;
}

}  // namespace Dynarmic::Backend::Arm64
