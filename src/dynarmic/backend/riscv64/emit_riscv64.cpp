/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/riscv64/emit_riscv64.h"

#include <biscuit/assembler.hpp>

#include "dynarmic/backend/riscv64/a32_jitstate.h"
#include "dynarmic/ir/basic_block.h"

namespace Dynarmic::Backend::RV64 {

EmittedBlockInfo EmitRV64(biscuit::Assembler& as, [[maybe_unused]] IR::Block block) {
    using namespace biscuit;

    EmittedBlockInfo ebi;
    ebi.entry_point = reinterpret_cast<CodePtr>(as.GetCursorPointer());

    as.ADDIW(a0, zero, 8);
    as.SW(a0, offsetof(A32JitState, regs) + 0 * sizeof(u32), a1);

    as.ADDIW(a0, zero, 2);
    as.SW(a0, offsetof(A32JitState, regs) + 1 * sizeof(u32), a1);
    as.SW(a0, offsetof(A32JitState, regs) + 15 * sizeof(u32), a1);

    ptrdiff_t offset = reinterpret_cast<CodePtr>(as.GetCursorPointer()) - ebi.entry_point;
    ebi.relocations.emplace_back(Relocation{offset, LinkTarget::ReturnFromRunCode});
    as.NOP();

    ebi.size = reinterpret_cast<CodePtr>(as.GetCursorPointer()) - ebi.entry_point;
    return ebi;
}

}  // namespace Dynarmic::Backend::RV64
