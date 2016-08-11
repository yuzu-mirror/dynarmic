/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

bool ArmTranslatorVisitor::arm_CPS() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_MRS() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_MSR() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_RFE() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SETEND(bool E) {
    // SETEND {BE,LE}
    ir.SetTerm(ir.current_location.AdvancePC(4).SetEFlag(E));
    return false;
}

bool ArmTranslatorVisitor::arm_SRS() {
    return InterpretThisInstruction();
}


} // namespace Arm
} // namespace Dynarmic
