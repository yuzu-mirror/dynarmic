/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_CLREX() {
    ir.ClearExclusive();
    return true;
}

bool ThumbTranslatorVisitor::thumb32_DMB([[maybe_unused]] Imm<4> option) {
    ir.DataMemoryBarrier();
    return true;
}

bool ThumbTranslatorVisitor::thumb32_DSB([[maybe_unused]] Imm<4> option) {
    ir.DataSynchronizationBarrier();
    return true;
}

bool ThumbTranslatorVisitor::thumb32_ISB([[maybe_unused]] Imm<4> option) {
    ir.InstructionSynchronizationBarrier();
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 4));
    ir.SetTerm(IR::Term::ReturnToDispatch{});
    return false;
}

bool ThumbTranslatorVisitor::thumb32_UDF() {
    return thumb16_UDF();
}

} // namespace Dynarmic::A32
