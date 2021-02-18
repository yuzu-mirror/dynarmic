/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_TST_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(imm_carry.carry);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_AND_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    ASSERT_MSG(!(d == Reg::PC && S), "Decode error");
    if ((d == Reg::PC && !S) || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool ThumbTranslatorVisitor::thumb32_BIC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Not(ir.Imm32(imm_carry.imm32)));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool ThumbTranslatorVisitor::thumb32_MOV_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.Imm32(imm_carry.imm32);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool ThumbTranslatorVisitor::thumb32_ORR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    ASSERT_MSG(n != Reg::PC, "Decode error");
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.Or(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

} // namespace Dynarmic::A32
