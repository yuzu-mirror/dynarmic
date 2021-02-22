/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_BXJ(Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    return thumb16_BX(m);
}

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
    ir.UpdateUpperLocationDescriptor();
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 4));
    ir.SetTerm(IR::Term::ReturnToDispatch{});
    return false;
}

bool ThumbTranslatorVisitor::thumb32_NOP() {
    return thumb16_NOP();
}

bool ThumbTranslatorVisitor::thumb32_SEV() {
    return thumb16_SEV();
}

bool ThumbTranslatorVisitor::thumb32_SEVL() {
    return thumb16_SEVL();
}

bool ThumbTranslatorVisitor::thumb32_UDF() {
    return thumb16_UDF();
}

bool ThumbTranslatorVisitor::thumb32_WFE() {
    return thumb16_WFE();
}

bool ThumbTranslatorVisitor::thumb32_WFI() {
    return thumb16_WFI();
}

bool ThumbTranslatorVisitor::thumb32_YIELD() {
    return thumb16_YIELD();
}

} // namespace Dynarmic::A32
