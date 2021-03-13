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

bool ThumbTranslatorVisitor::thumb32_TBB(Reg n, Reg m) {
    return TableBranch(*this, n, m, false);
}

bool ThumbTranslatorVisitor::thumb32_TBH(Reg n, Reg m) {
    return TableBranch(*this, n, m, true);
}

} // namespace Dynarmic::A32
