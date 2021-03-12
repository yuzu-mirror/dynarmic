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

static bool StoreByteImmediate(ThumbTranslatorVisitor& v, Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    const auto imm32 = imm8.ZeroExtend();
    const auto reg_n = v.ir.GetRegister(n);
    const auto reg_t = v.ir.GetRegister(t);

    const IR::U32 offset_address = U ? v.ir.Add(reg_n, v.ir.Imm32(imm32))
                                     : v.ir.Sub(reg_n, v.ir.Imm32(imm32));
    const IR::U32 address = P ? offset_address
                              : reg_n;
    const IR::U8 data = v.ir.LeastSignificantByte(reg_t);

    v.ir.WriteMemory8(address, data);
    if (W) {
        v.ir.SetRegister(n, offset_address);
    }

    return true;
}

bool ThumbTranslatorVisitor::thumb32_STRB_imm_1(Reg n, Reg t, bool P, bool U, Imm<8> imm8) {
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC || n == t) {
        return UnpredictableInstruction();
    }
    return StoreByteImmediate(*this, n, t, P, U, true, imm8);
}

bool ThumbTranslatorVisitor::thumb32_STRB_imm_2(Reg n, Reg t, Imm<8> imm8) {
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }
    return StoreByteImmediate(*this, n, t, true, false, false, imm8);
}

bool ThumbTranslatorVisitor::thumb32_STRB_imm_3(Reg n, Reg t, Imm<12> imm12) {
    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = imm12.ZeroExtend();
    const auto reg_n = ir.GetRegister(n);
    const auto reg_t = ir.GetRegister(t);
    const auto address = ir.Add(reg_n, ir.Imm32(imm32));
    const auto data = ir.LeastSignificantByte(reg_t);

    ir.WriteMemory8(address, data);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_STRBT(Reg n, Reg t, Imm<8> imm8) {
    // TODO: Add an unpredictable instruction path if this
    //       is executed in hypervisor mode if we ever support
    //       privileged execution levels.

    if (n == Reg::PC) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    // Treat this as a normal STRB, given we don't support
    // execution levels other than EL0 currently.
    return StoreByteImmediate(*this, n, t, true, true, false, imm8);
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
