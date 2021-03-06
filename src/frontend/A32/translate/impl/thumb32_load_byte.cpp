/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <dynarmic/A32/config.h>
#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {
static bool PLDHandler(ThumbTranslatorVisitor& v, bool W) {
    if (!v.options.hook_hint_instructions) {
        return true;
    }

    const auto exception = W ? Exception::PreloadDataWithIntentToWrite
                             : Exception::PreloadData;
    return v.RaiseException(exception);
}

static bool PLIHandler(ThumbTranslatorVisitor& v) {
    if (!v.options.hook_hint_instructions) {
        return true;
    }

    return v.RaiseException(Exception::PreloadInstruction);
}

bool ThumbTranslatorVisitor::thumb32_PLD_lit([[maybe_unused]] bool U,
                                             [[maybe_unused]] Imm<12> imm12) {
    return PLDHandler(*this, false);
}

bool ThumbTranslatorVisitor::thumb32_PLD_imm8(bool W,
                                              [[maybe_unused]] Reg n,
                                              [[maybe_unused]] Imm<8> imm8) {
    return PLDHandler(*this, W);
}

bool ThumbTranslatorVisitor::thumb32_PLD_imm12(bool W,
                                               [[maybe_unused]] Reg n,
                                               [[maybe_unused]] Imm<12> imm12) {
    return PLDHandler(*this, W);
}

bool ThumbTranslatorVisitor::thumb32_PLD_reg(bool W,
                                             [[maybe_unused]] Reg n,
                                             [[maybe_unused]] Imm<2> imm2,
                                             Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    return PLDHandler(*this, W);
}

bool ThumbTranslatorVisitor::thumb32_PLI_lit([[maybe_unused]] bool U,
                                             [[maybe_unused]] Imm<12> imm12) {
    return PLIHandler(*this);
}

bool ThumbTranslatorVisitor::thumb32_PLI_imm8([[maybe_unused]] Reg n,
                                              [[maybe_unused]] Imm<8> imm8) {
    return PLIHandler(*this);
}

bool ThumbTranslatorVisitor::thumb32_PLI_imm12([[maybe_unused]] Reg n,
                                               [[maybe_unused]] Imm<12> imm12) {
    return PLIHandler(*this);
}

bool ThumbTranslatorVisitor::thumb32_PLI_reg([[maybe_unused]] Reg n,
                                             [[maybe_unused]] Imm<2> imm2,
                                             Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    return PLIHandler(*this);
}

bool ThumbTranslatorVisitor::thumb32_LDRB_lit(bool U, Reg t, Imm<12> imm12) {
    const u32 imm32 = imm12.ZeroExtend();
    const u32 base = ir.AlignPC(4);
    const u32 address = U ? (base + imm32) : (base - imm32);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(ir.Imm32(address)));

    ir.SetRegister(t, data);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_LDRB_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (t == Reg::PC && W) {
        return UnpredictableInstruction();
    }
    if (W && n == t) {
        return UnpredictableInstruction();
    }
    if (!P && !W) {
        return UndefinedInstruction();
    }

    const u32 imm32 = imm8.ZeroExtend();
    const IR::U32 reg_n = ir.GetRegister(n);
    const IR::U32 offset_address = U ? ir.Add(reg_n, ir.Imm32(imm32))
                                     : ir.Sub(reg_n, ir.Imm32(imm32));
    const IR::U32 address = P ? offset_address : reg_n;
    const IR::U32 data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    if (W) {
        ir.SetRegister(n, offset_address);
    }
    return true;
}

bool ThumbTranslatorVisitor::thumb32_LDRB_imm12(Reg n, Reg t, Imm<12> imm12) {
    const auto imm32 = imm12.ZeroExtend();
    const auto reg_n = ir.GetRegister(n);
    const auto address = ir.Add(reg_n, ir.Imm32(imm32));
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_LDRB_reg(Reg n, Reg t, Imm<2> imm2, Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_n = ir.GetRegister(n);
    const auto reg_m = ir.GetRegister(m);
    const auto offset = ir.LogicalShiftLeft(reg_m, ir.Imm8(imm2.ZeroExtend<u8>()));
    const auto address = ir.Add(reg_n, offset);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address));

    ir.SetRegister(t, data);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_LDRBT(Reg n, Reg t, Imm<8> imm8) {
    // TODO: Add an unpredictable instruction path if this
    //       is executed in hypervisor mode if we ever support
    //       privileged execution modes.

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    // Treat it as a normal LDRB, given we don't support
    // execution levels other than EL0 currently.
    return thumb32_LDRB_imm8(n, t, true, true, false, imm8);
}

} // namespace Dynarmic::A32
