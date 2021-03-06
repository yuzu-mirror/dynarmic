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

} // namespace Dynarmic::A32
