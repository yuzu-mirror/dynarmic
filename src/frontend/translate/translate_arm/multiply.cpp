/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

// Multiply (Normal) instructions
bool ArmTranslatorVisitor::arm_MLA(Cond cond, bool S, Reg d, Reg a, Reg m, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_MUL(Cond cond, bool S, Reg d, Reg m, Reg n) {
    return InterpretThisInstruction();
}


// Multiply (Long) instructions
bool ArmTranslatorVisitor::arm_SMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UMAAL(Cond cond, Reg dHi, Reg dLo, Reg m, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
    return InterpretThisInstruction();
}


// Multiply (Halfword) instructions
bool ArmTranslatorVisitor::arm_SMLALxy(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, bool N, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMLAxy(Cond cond, Reg d, Reg a, Reg m, bool M, bool N, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMULxy(Cond cond, Reg d, Reg m, bool M, bool N, Reg n) {
    return InterpretThisInstruction();
}


// Multiply (word by halfword) instructions
bool ArmTranslatorVisitor::arm_SMLAWy(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMULWy(Cond cond, Reg d, Reg m, bool M, Reg n) {
    return InterpretThisInstruction();
}


// Multiply (Most significant word) instructions
bool ArmTranslatorVisitor::arm_SMMLA(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMMLS(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMMUL(Cond cond, Reg d, Reg m, bool R, Reg n) {
    return InterpretThisInstruction();
}


// Multiply (Dual) instructions
bool ArmTranslatorVisitor::arm_SMLAD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMLALD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMLSD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMLSLD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMUAD(Cond cond, Reg d, Reg m, bool M, Reg n) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SMUSD(Cond cond, Reg d, Reg m, bool M, Reg n) {
    return InterpretThisInstruction();
}

} // namespace Arm
} // namespace Dynarmic
