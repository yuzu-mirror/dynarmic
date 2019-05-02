/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <dynarmic/A32/config.h>
#include "translate_arm.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::arm_PLD() {
    return RaiseException(Exception::PreloadData);
}

bool ArmTranslatorVisitor::arm_SEV() {
    return RaiseException(Exception::SendEvent);
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
