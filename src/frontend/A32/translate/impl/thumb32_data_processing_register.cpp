/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {
static IR::U32 Rotate(A32::IREmitter& ir, Reg m, SignExtendRotation rotate) {
    const u8 rotate_by = static_cast<u8>(static_cast<size_t>(rotate) * 8);
    return ir.RotateRight(ir.GetRegister(m), ir.Imm8(rotate_by), ir.Imm1(0)).result;
}

bool ThumbTranslatorVisitor::thumb32_SXTB16(Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto low_byte = ir.And(rotated, ir.Imm32(0x00FF00FF));
    const auto sign_bit = ir.And(rotated, ir.Imm32(0x00800080));
    const auto result = ir.Or(low_byte, ir.Mul(sign_bit, ir.Imm32(0x1FE)));

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SXTAB16(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto low_byte = ir.And(rotated, ir.Imm32(0x00FF00FF));
    const auto sign_bit = ir.And(rotated, ir.Imm32(0x00800080));
    const auto addend = ir.Or(low_byte, ir.Mul(sign_bit, ir.Imm32(0x1FE)));
    const auto result = ir.PackedAddU16(addend, ir.GetRegister(n)).result;

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SXTH(Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(rotated));

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SXTAH(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.SignExtendHalfToWord(ir.LeastSignificantHalf(rotated)));

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UXTH(Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(rotated));

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UXTAH(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(rotated)));

    ir.SetRegister(d, result);
    return true;
}

} // namespace Dynarmic::A32
