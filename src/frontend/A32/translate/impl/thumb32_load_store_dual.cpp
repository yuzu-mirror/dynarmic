/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"
#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {
static bool ITBlockCheck(const A32::IREmitter& ir) {
    return ir.current_location.IT().IsInITBlock() && !ir.current_location.IT().IsLastInITBlock();
}

static bool TableBranch(ThumbTranslatorVisitor& v, Reg n, Reg m, bool half) {
    if (m == Reg::PC) {
        return v.UnpredictableInstruction();
    }
    if (ITBlockCheck(v.ir)) {
        return v.UnpredictableInstruction();
    }

    const auto reg_m = v.ir.GetRegister(m);
    const auto reg_n = v.ir.GetRegister(n);

    IR::U32 halfwords;
    if (half) {
        const auto data = v.ir.ReadMemory16(v.ir.Add(reg_n, v.ir.LogicalShiftLeft(reg_m, v.ir.Imm8(1))));
        halfwords = v.ir.ZeroExtendToWord(data);
    } else {
        halfwords = v.ir.ZeroExtendToWord(v.ir.ReadMemory8(v.ir.Add(reg_n, reg_m)));
    }

    const auto current_pc = v.ir.Imm32(v.ir.PC());
    const auto branch_value = v.ir.Add(current_pc, v.ir.Add(halfwords, halfwords));

    v.ir.UpdateUpperLocationDescriptor();
    v.ir.BranchWritePC(branch_value);
    v.ir.SetTerm(IR::Term::FastDispatchHint{});
    return false;
}

static bool LoadDualImmediate(ThumbTranslatorVisitor& v, bool P, bool U, bool W,
                              Reg n, Reg t, Reg t2, Imm<8> imm8) {
    if (W && (n == t || n == t2)) {
        return v.UnpredictableInstruction();
    }
    if (t == Reg::PC || t2 == Reg::PC || t == t2) {
        return v.UnpredictableInstruction();
    }

    const u32 imm = imm8.ZeroExtend() << 2;
    const IR::U32 reg_n = v.ir.GetRegister(n);
    const IR::U32 offset_address = U ? v.ir.Add(reg_n, v.ir.Imm32(imm))
                                     : v.ir.Sub(reg_n, v.ir.Imm32(imm));
    const IR::U32 address_1 = P ? offset_address
                                : reg_n;
    const IR::U32 address_2 = v.ir.Add(address_1, v.ir.Imm32(4));

    v.ir.SetRegister(t, v.ir.ReadMemory32(address_1));
    v.ir.SetRegister(t2, v.ir.ReadMemory32(address_2));

    if (W) {
        v.ir.SetRegister(n, offset_address);
    }
    return true;
}

static bool LoadDualLiteral(ThumbTranslatorVisitor& v, bool U, bool W, Reg t, Reg t2, Imm<8> imm8) {
    if (t == Reg::PC || t2 == Reg::PC || t == t2) {
        return v.UnpredictableInstruction();
    }
    if (W) {
        return v.UnpredictableInstruction();
    }

    const auto imm = imm8.ZeroExtend() << 2;
    const auto address_1 = U ? v.ir.Add(v.ir.Imm32(v.ir.AlignPC(4)), v.ir.Imm32(imm))
                             : v.ir.Sub(v.ir.Imm32(v.ir.AlignPC(4)), v.ir.Imm32(imm));
    const auto address_2 = v.ir.Add(address_1, v.ir.Imm32(4));

    v.ir.SetRegister(t, v.ir.ReadMemory32(address_1));
    v.ir.SetRegister(t2, v.ir.ReadMemory32(address_2));
    return true;
}

static bool StoreDual(ThumbTranslatorVisitor& v, bool P, bool U, bool W, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    if (W && (n == t || n == t2)) {
        return v.UnpredictableInstruction();
    }
    if (n == Reg::PC || t == Reg::PC || t2 == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    const u32 imm = imm8.ZeroExtend() << 2;
    const IR::U32 reg_n = v.ir.GetRegister(n);
    const IR::U32 reg_t = v.ir.GetRegister(t);
    const IR::U32 reg_t2 = v.ir.GetRegister(t2);

    const IR::U32 offset_address = U ? v.ir.Add(reg_n, v.ir.Imm32(imm))
                                     : v.ir.Sub(reg_n, v.ir.Imm32(imm));
    const IR::U32 address_1 = P ? offset_address
                                : reg_n;
    const IR::U32 address_2 = v.ir.Add(address_1, v.ir.Imm32(4));

    v.ir.WriteMemory32(address_1, reg_t);
    v.ir.WriteMemory32(address_2, reg_t2);

    if (W) {
        v.ir.SetRegister(n, offset_address);
    }
    return true;
}

bool ThumbTranslatorVisitor::thumb32_LDRD_imm_1(bool U, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    return LoadDualImmediate(*this, false, U, true, n, t, t2, imm8);
}

bool ThumbTranslatorVisitor::thumb32_LDRD_imm_2(bool U, bool W, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    return LoadDualImmediate(*this, true, U, W, n, t, t2, imm8);
}

bool ThumbTranslatorVisitor::thumb32_LDRD_lit_1(bool U, Reg t, Reg t2, Imm<8> imm8) {
    return LoadDualLiteral(*this, U, true, t, t2, imm8);
}

bool ThumbTranslatorVisitor::thumb32_LDRD_lit_2(bool U, bool W, Reg t, Reg t2, Imm<8> imm8) {
    return LoadDualLiteral(*this, U, W, t, t2, imm8);
}

bool ThumbTranslatorVisitor::thumb32_STRD_imm_1(bool U, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    return StoreDual(*this, false, U, true, n, t, t2, imm8);
}

bool ThumbTranslatorVisitor::thumb32_STRD_imm_2(bool U, bool W, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    return StoreDual(*this, true, U, W, n, t, t2, imm8);
}

bool ThumbTranslatorVisitor::thumb32_TBB(Reg n, Reg m) {
    return TableBranch(*this, n, m, false);
}

bool ThumbTranslatorVisitor::thumb32_TBH(Reg n, Reg m) {
    return TableBranch(*this, n, m, true);
}

} // namespace Dynarmic::A32
