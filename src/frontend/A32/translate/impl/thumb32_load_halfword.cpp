/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_LDRH_lit(bool U, Reg t, Imm<12> imm12) {
    const auto imm32 = imm12.ZeroExtend();
    const auto base = ir.AlignPC(4);
    const auto address = U ? (base + imm32) : (base - imm32);
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(ir.Imm32(address)));

    ir.SetRegister(t, data);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_LDRH_reg(Reg n, Reg t, Imm<2> imm2, Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const IR::U32 reg_m = ir.GetRegister(m);
    const IR::U32 reg_n = ir.GetRegister(n);
    const IR::U32 offset = ir.LogicalShiftLeft(reg_m, ir.Imm8(imm2.ZeroExtend<u8>()));
    const IR::U32 address = ir.Add(reg_n, offset);
    const IR::U32 data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));

    ir.SetRegister(t, data);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_LDRH_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!P && !W) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC && W) {
        return UnpredictableInstruction();
    }
    if (W && n == t) {
        return UnpredictableInstruction();
    }

    const u32 imm32 = imm8.ZeroExtend();
    const IR::U32 reg_n = ir.GetRegister(n);
    const IR::U32 offset_address = U ? ir.Add(reg_n, ir.Imm32(imm32))
                                     : ir.Sub(reg_n, ir.Imm32(imm32));
    const IR::U32 address = P ? offset_address
                              : reg_n;
    const IR::U32 data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));

    if (W) {
        ir.SetRegister(n, offset_address);
    }

    ir.SetRegister(t, data);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_LDRH_imm12(Reg n, Reg t, Imm<12> imm12) {
    const auto imm32 = imm12.ZeroExtend();
    const auto reg_n = ir.GetRegister(n);
    const auto address = ir.Add(reg_n, ir.Imm32(imm32));
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address));

    ir.SetRegister(t, data);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_LDRHT(Reg n, Reg t, Imm<8> imm8) {
    // TODO: Add an unpredictable instruction path if this
    //       is executed in hypervisor mode if we ever support
    //       privileged execution levels.

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    // Treat it as a normal LDRH, given we don't support
    // execution levels other than EL0 currently.
    return thumb32_LDRH_imm8(n, t, true, true, false, imm8);
}

} // namespace Dynarmic::A32
