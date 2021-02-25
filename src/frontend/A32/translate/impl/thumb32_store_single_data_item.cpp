/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

template <typename StoreRegFn>
static bool StoreRegister(ThumbTranslatorVisitor& v, Reg n, Reg t, Imm<2> imm2, Reg m, StoreRegFn store_fn) {
    if (n == Reg::PC) {
        return v.UndefinedInstruction();
    }

    if (t == Reg::PC || m == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    const auto reg_m = v.ir.GetRegister(m);
    const auto reg_n = v.ir.GetRegister(n);
    const auto reg_t = v.ir.GetRegister(t);

    const auto shift_amount = v.ir.Imm8(static_cast<u8>(imm2.ZeroExtend()));
    const auto offset = v.ir.LogicalShiftLeft(reg_m, shift_amount);
    const auto offset_address = v.ir.Add(reg_n, offset);

    store_fn(offset_address, reg_t);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_STRB(Reg n, Reg t, Imm<2> imm2, Reg m) {
    return StoreRegister(*this, n, t, imm2, m, [this](const IR::U32& offset_address, const IR::U32& data) {
        ir.WriteMemory8(offset_address, ir.LeastSignificantByte(data));
    });
}

bool ThumbTranslatorVisitor::thumb32_STRH(Reg n, Reg t, Imm<2> imm2, Reg m) {
    return StoreRegister(*this, n, t, imm2, m, [this](const IR::U32& offset_address, const IR::U32& data) {
        ir.WriteMemory16(offset_address, ir.LeastSignificantHalf(data));
    });
}

bool ThumbTranslatorVisitor::thumb32_STR_reg(Reg n, Reg t, Imm<2> imm2, Reg m) {
    return StoreRegister(*this, n, t, imm2, m, [this](const IR::U32& offset_address, const IR::U32& data) {
        ir.WriteMemory32(offset_address, data);
    });
}

} // namespace Dynarmic::A32
