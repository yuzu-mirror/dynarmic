/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_MLA(Reg n, Reg a, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_a = ir.GetRegister(a);
    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(ir.Mul(reg_n, reg_m), reg_a);

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_MLS(Reg n, Reg a, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_a = ir.GetRegister(a);
    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Sub(reg_a, ir.Mul(reg_n, reg_m));

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_MUL(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Mul(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SMLAXY(Reg n, Reg a, Reg d, bool N, bool M, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n16 = N ? ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result
                          : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 m16 = M ? ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result
                          : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    const IR::U32 product = ir.Mul(n16, m16);
    const auto result_overflow = ir.AddWithCarry(product, ir.GetRegister(a), ir.Imm1(0));

    ir.SetRegister(d, result_overflow.result);
    ir.OrQFlag(result_overflow.overflow);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SMULXY(Reg n, Reg d, bool N, bool M, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto n32 = ir.GetRegister(n);
    const auto m32 = ir.GetRegister(m);
    const auto n16 = N ? ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result
                       : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const auto m16 = M ? ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result
                       : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    const auto result = ir.Mul(n16, m16);

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_USAD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAbsDiffSumS8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_USADA8(Reg n, Reg a, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_a = ir.GetRegister(a);
    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto tmp = ir.PackedAbsDiffSumS8(reg_n, reg_m);
    const auto result = ir.AddWithCarry(reg_a, tmp, ir.Imm1(0));

    ir.SetRegister(d, result.result);
    return true;
}

} // namespace Dynarmic::A32
