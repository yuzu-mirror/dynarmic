/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <dynarmic/A32/config.h>
#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::arm_PLD_imm([[maybe_unused]] bool add,
                                       bool R,
                                       [[maybe_unused]] Reg n,
                                       [[maybe_unused]] Imm<12> imm12) {
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
    const auto exception = R ? Exception::PreloadData
                             : Exception::PreloadDataWithIntentToWrite;
    return RaiseException(exception);
}

bool ArmTranslatorVisitor::arm_SEV() {
    return RaiseException(Exception::SendEvent);
}

bool ArmTranslatorVisitor::arm_SEVL() {
    return RaiseException(Exception::SendEventLocal);
}

bool ArmTranslatorVisitor::arm_WFE() {
    return RaiseException(Exception::WaitForEvent);
}

bool ArmTranslatorVisitor::arm_WFI() {
    return RaiseException(Exception::WaitForInterrupt);
}

bool ArmTranslatorVisitor::arm_YIELD() {
    return RaiseException(Exception::Yield);
}

} // namespace Dynarmic::A32
