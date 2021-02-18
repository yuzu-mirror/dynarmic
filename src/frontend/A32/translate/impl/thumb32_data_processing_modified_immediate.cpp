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

    const auto imm_carry = ThumbExpandImm_C(rotate, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(imm_carry.carry);
    return true;
}

} // namespace Dynarmic::A32
