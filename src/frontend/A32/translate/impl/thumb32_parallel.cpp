/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_SADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddS16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddU16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

} // namespace Dynarmic::A32
