/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

// Parallel Add/Subtract (Modulo arithmetic) instructions
bool ArmTranslatorVisitor::arm_SADD8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SADD16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SASX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SSAX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SSUB8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SSUB16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UADD8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UADD16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UASX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_USAX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_USUB8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_USUB16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}


// Parallel Add/Subtract (Saturating) instructions
bool ArmTranslatorVisitor::arm_QADD8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_QADD16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_QASX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_QSAX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_QSUB8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_QSUB16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UQADD8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UQADD16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UQASX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UQSAX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UQSUB8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UQSUB16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}


// Parallel Add/Subtract (Halving) instructions
bool ArmTranslatorVisitor::arm_SHADD8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SHADD16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SHASX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SHSAX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SHSUB8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SHSUB16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UHADD8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UHADD16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UHASX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UHSAX(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UHSUB8(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UHSUB16(Cond cond, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

} // namespace Arm
} // namespace Dynarmic
