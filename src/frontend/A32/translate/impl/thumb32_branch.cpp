/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

// BL <label>
bool ThumbTranslatorVisitor::thumb32_BL_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo) {
    const Imm<1> i1{j1 == S};
    const Imm<1> i2{j2 == S};

    if (ir.current_location.IT().IsInITBlock() && !ir.current_location.IT().IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.SetRegister(Reg::LR, ir.Imm32((ir.current_location.PC() + 4) | 1));

    const s32 imm32 = static_cast<s32>((concatenate(S, i1, i2, hi, lo).SignExtend<u32>() << 1) + 4);
    const auto new_location = ir.current_location
                                .AdvancePC(imm32)
                                .AdvanceIT();
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

// BLX <label>
bool ThumbTranslatorVisitor::thumb32_BLX_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo) {
    const Imm<1> i1{j1 == S};
    const Imm<1> i2{j2 == S};

    if (ir.current_location.IT().IsInITBlock() && !ir.current_location.IT().IsLastInITBlock()) {
        return UnpredictableInstruction();
    }

    if (lo.Bit<0>()) {
        return UnpredictableInstruction();
    }

    ir.PushRSB(ir.current_location.AdvancePC(4));
    ir.SetRegister(Reg::LR, ir.Imm32((ir.current_location.PC() + 4) | 1));

    const s32 imm32 = static_cast<s32>(concatenate(S, i1, i2, hi, lo).SignExtend<u32>() << 1);
    const auto new_location = ir.current_location
                                .SetPC(ir.AlignPC(4) + imm32)
                                .SetTFlag(false)
                                .AdvanceIT();
    ir.SetTerm(IR::Term::LinkBlock{new_location});
    return false;
}

} // namespace Dynarmic::A32
