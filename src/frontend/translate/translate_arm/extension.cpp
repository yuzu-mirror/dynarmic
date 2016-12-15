/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

IR::Value ArmTranslatorVisitor::SignZeroExtendRor(Reg m, SignExtendRotation rotate) {
    IR::Value rotated, reg_m = ir.GetRegister(m);
    switch (rotate) {
    case SignExtendRotation::ROR_0:
        rotated = reg_m;
        break;
    case SignExtendRotation::ROR_8:
        rotated = ir.RotateRight(reg_m, ir.Imm8(8), ir.Imm1(0)).result;
        break;
    case SignExtendRotation::ROR_16:
        rotated = ir.RotateRight(reg_m, ir.Imm8(16), ir.Imm1(0)).result;
        break;
    case SignExtendRotation::ROR_24:
        rotated = ir.RotateRight(reg_m, ir.Imm8(24), ir.Imm1(0)).result;
    }
    return rotated;
}

bool ArmTranslatorVisitor::arm_SXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto rotated = SignZeroExtendRor(m, rotate);
        auto reg_n = ir.GetRegister(n);
        auto result = ir.Add(reg_n, ir.SignExtendByteToWord(ir.LeastSignificantByte(rotated)));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_SXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    UNUSED(cond, n, d, rotate, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto rotated = SignZeroExtendRor(m, rotate);
        auto reg_n = ir.GetRegister(n);
        auto result = ir.Add(reg_n, ir.SignExtendHalfToWord(ir.LeastSignificantHalf(rotated)));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_SXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto rotated = SignZeroExtendRor(m, rotate);
        auto result = ir.SignExtendByteToWord(ir.LeastSignificantByte(rotated));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_SXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    UNUSED(cond, d, rotate, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto rotated = SignZeroExtendRor(m, rotate);
        auto result = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(rotated));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto rotated = SignZeroExtendRor(m, rotate);
        auto reg_n = ir.GetRegister(n);
        auto result = ir.Add(reg_n, ir.ZeroExtendByteToWord(ir.LeastSignificantByte(rotated)));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    UNUSED(cond, n, d, rotate, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto rotated = SignZeroExtendRor(m, rotate);
        auto reg_n = ir.GetRegister(n);
        auto result = ir.Add(reg_n, ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(rotated)));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto rotated = SignZeroExtendRor(m, rotate);
        auto result = ir.ZeroExtendByteToWord(ir.LeastSignificantByte(rotated));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto rotated = SignZeroExtendRor(m, rotate);
        auto lower_half = ir.ZeroExtendByteToWord(ir.LeastSignificantByte(rotated));
        auto upper_half = ir.And(rotated, ir.Imm32(0x00FF0000));
        auto result = ir.Or(lower_half, upper_half);
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto rotated = SignZeroExtendRor(m, rotate);
        auto result = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(rotated));
        ir.SetRegister(d, result);
    }
    return true;
}

} // namespace Arm
} // namespace Dynarmic
