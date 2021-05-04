/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::thumb32_BXJ(Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    return thumb16_BX(m);
}

bool TranslatorVisitor::thumb32_CLREX() {
    ir.ClearExclusive();
    return true;
}

bool TranslatorVisitor::thumb32_DMB(Imm<4> /*option*/) {
    ir.DataMemoryBarrier();
    return true;
}

bool TranslatorVisitor::thumb32_DSB(Imm<4> /*option*/) {
    ir.DataSynchronizationBarrier();
    return true;
}

bool TranslatorVisitor::thumb32_ISB(Imm<4> /*option*/) {
    ir.InstructionSynchronizationBarrier();
    ir.UpdateUpperLocationDescriptor();
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 4));
    ir.SetTerm(IR::Term::ReturnToDispatch{});
    return false;
}

bool TranslatorVisitor::thumb32_NOP() {
    return thumb16_NOP();
}

bool TranslatorVisitor::thumb32_SEV() {
    return thumb16_SEV();
}

bool TranslatorVisitor::thumb32_SEVL() {
    return thumb16_SEVL();
}

bool TranslatorVisitor::thumb32_UDF() {
    return thumb16_UDF();
}

bool TranslatorVisitor::thumb32_WFE() {
    return thumb16_WFE();
}

bool TranslatorVisitor::thumb32_WFI() {
    return thumb16_WFI();
}

bool TranslatorVisitor::thumb32_YIELD() {
    return thumb16_YIELD();
}

bool TranslatorVisitor::thumb32_MRS_reg(bool read_spsr, Reg d) {
    if (d == Reg::R15) {
        return UnpredictableInstruction();
    }

    // TODO: Revisit when implementing more than user mode.

    if (read_spsr) {
        return UndefinedInstruction();
    }

    ir.SetRegister(d, ir.GetCpsr());
    return true;
}

} // namespace Dynarmic::A32
