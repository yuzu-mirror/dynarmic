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
    ebi.entry_point = reinterpret_cast<void*>(as.GetCursorPointer());

    as.ADDIW(a0, zero, 8);
    as.SW(a0, offsetof(A32JitState, regs) + 0 * sizeof(u32), a1);

    as.ADDIW(a0, zero, 2);
    as.SW(a0, offsetof(A32JitState, regs) + 1 * sizeof(u32), a1);
    as.SW(a0, offsetof(A32JitState, regs) + 15 * sizeof(u32), a1);

    ebi.relocations[reinterpret_cast<char*>(as.GetCursorPointer()) - reinterpret_cast<char*>(ebi.entry_point)] = LinkTarget::ReturnFromRunCode;
    as.NOP();

    ebi.size = reinterpret_cast<size_t>(as.GetCursorPointer()) - reinterpret_cast<size_t>(ebi.entry_point);
    return ebi;
}

}  // namespace Dynarmic::Backend::RV64
