/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_TST_reg(Reg n, Imm<3> imm3, Imm<2> imm2, ShiftType type, Reg m) {
    if (n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), type, imm3, imm2, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), shifted.result);

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(shifted.carry);
    return true;
}

} // namespace Dynarmic::A32
