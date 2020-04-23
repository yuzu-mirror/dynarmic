/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <dynarmic/A32/config.h>
#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::arm_PLD_imm([[maybe_unused]] bool add,
                                       bool R,
                                       [[maybe_unused]] Reg n,
                                       [[maybe_unused]] Imm<12> imm12) {
    if (!options.hook_hint_instructions) {
        return true;
    }

    const auto exception = R ? Exception::PreloadData
                             : Exception::PreloadDataWithIntentToWrite;
    return RaiseException(exception);
}

bool ArmTranslatorVisitor::arm_PLD_reg([[maybe_unused]] bool add,
                                       bool R,
                                       [[maybe_unused]] Reg n,
                                       [[maybe_unused]] Imm<5> imm5,
                                       [[maybe_unused]] ShiftType shift,
                                       [[maybe_unused]] Reg m) {
    if (!options.hook_hint_instructions) {
        return true;
    }

    const auto exception = R ? Exception::PreloadData
                             : Exception::PreloadDataWithIntentToWrite;
    return RaiseException(exception);
}

bool ArmTranslatorVisitor::arm_SEV() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::SendEvent);
}

bool ArmTranslatorVisitor::arm_SEVL() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::SendEventLocal);
}

bool ArmTranslatorVisitor::arm_WFE() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::WaitForEvent);
}

bool ArmTranslatorVisitor::arm_WFI() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::WaitForInterrupt);
}

bool ArmTranslatorVisitor::arm_YIELD() {
    if (!options.hook_hint_instructions) {
        return true;
    }

    return RaiseException(Exception::Yield);
}

} // namespace Dynarmic::A32
