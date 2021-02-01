/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_CLZ(Reg n, Reg d, Reg m) {
    if (m != n || d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto result = ir.CountLeadingZeros(reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SEL(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSelect(ir.GetGEFlags(), reg_m, reg_n);

    ir.SetRegister(d, result);
    return true;
}

} // namespace Dynarmic::A32
