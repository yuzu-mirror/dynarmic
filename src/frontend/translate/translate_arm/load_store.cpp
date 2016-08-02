/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

bool ArmTranslatorVisitor::arm_LDR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm12 imm12) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm12 imm12) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRBT() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRHT() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRSB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRSB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRSBT() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRSH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRSH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRSHT() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDRT() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm12 imm12) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm12 imm12) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STRBT() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STRHT() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STRT() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDM(Cond cond, bool P, bool U, bool W, Reg n, RegList list) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDM_usr() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDM_eret() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STM(Cond cond, bool P, bool U, bool W, Reg n, RegList list) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_STM_usr() {
    return InterpretThisInstruction();
}

} // namespace Arm
} // namespace Dynarmic
