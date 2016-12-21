/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

// Saturation instructions

bool ArmTranslatorVisitor::arm_SSAT(Cond cond, Imm5 sat_imm, Reg d, Imm5 imm5, bool sh, Reg n) {
    if (d == Reg::PC || n == Reg::PC)
        return UnpredictableInstruction();

    size_t saturate_to = static_cast<size_t>(sat_imm) + 1;
    ShiftType shift = !sh ? ShiftType::LSL : ShiftType::ASR;

    // SSAT <Rd>, #<saturate_to>, <Rn>
    if (ConditionPassed(cond)) {
        auto operand = EmitImmShift(ir.GetRegister(n), shift, imm5, ir.GetCFlag());
        auto result = ir.SignedSaturation(operand.result, saturate_to);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_SSAT16(Cond cond, Imm4 sat_imm, Reg d, Reg n) {
    UNUSED(cond, sat_imm, d, n);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_USAT(Cond cond, Imm5 sat_imm, Reg d, Imm5 imm5, bool sh, Reg n) {
    if (d == Reg::PC || n == Reg::PC)
        return UnpredictableInstruction();

    size_t saturate_to = static_cast<size_t>(sat_imm);
    ShiftType shift = !sh ? ShiftType::LSL : ShiftType::ASR;

    // USAT <Rd>, #<saturate_to>, <Rn>
    if (ConditionPassed(cond)) {
        auto operand = EmitImmShift(ir.GetRegister(n), shift, imm5, ir.GetCFlag());
        auto result = ir.UnsignedSaturation(operand.result, saturate_to);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_USAT16(Cond cond, Imm4 sat_imm, Reg d, Reg n) {
    UNUSED(cond, sat_imm, d, n);
    return InterpretThisInstruction();
}

// Saturated Add/Subtract instructions

bool ArmTranslatorVisitor::arm_QADD(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    // QADD <Rd>, <Rm>, <Rn>
    if (ConditionPassed(cond)) {
        auto a = ir.GetRegister(m);
        auto b = ir.GetRegister(n);
        auto result = ir.SignedSaturatedAdd(a, b);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_QSUB(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    // QSUB <Rd>, <Rm>, <Rn>
    if (ConditionPassed(cond)) {
        auto a = ir.GetRegister(m);
        auto b = ir.GetRegister(n);
        auto result = ir.SignedSaturatedSub(a, b);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_QDADD(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    // QDADD <Rd>, <Rm>, <Rn>
    if (ConditionPassed(cond)) {
        auto a = ir.GetRegister(m);
        auto b = ir.GetRegister(n);
        auto doubled = ir.SignedSaturatedAdd(b, b);
        ir.OrQFlag(doubled.overflow);
        auto result = ir.SignedSaturatedAdd(a, doubled.result);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_QDSUB(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    // QDSUB <Rd>, <Rm>, <Rn>
    if (ConditionPassed(cond)) {
        auto a = ir.GetRegister(m);
        auto b = ir.GetRegister(n);
        auto doubled = ir.SignedSaturatedAdd(b, b);
        ir.OrQFlag(doubled.overflow);
        auto result = ir.SignedSaturatedSub(a, doubled.result);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}


} // namespace Arm
} // namespace Dynarmic
