/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

bool ArmTranslatorVisitor::arm_SXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    return InterpretThisInstruction();
}

} // namespace Arm
} // namespace Dynarmic
