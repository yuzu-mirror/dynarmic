/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

static IR::Value GetAddressingMode(IREmitter& ir, bool P, bool U, bool W, Reg n, IR::Value index) {
    IR::Value address;
    if (P) {
        // Pre-indexed addressing
        if (n == Reg::PC && index.IsImmediate()) {
            address = U ? ir.Imm32(ir.AlignPC(4) + index.GetU32()) : ir.Imm32(ir.AlignPC(4) - index.GetU32());
        } else {
            address = U ? ir.Add(ir.GetRegister(n), index) : ir.Sub(ir.GetRegister(n), index);
        }

        // Wrote calculated address back to the base register
        if (W) {
            ir.SetRegister(n, address);
        }
    } else {
        // Post-indexed addressing
        address = (n == Reg::PC) ? ir.Imm32(ir.AlignPC(4)) : ir.GetRegister(n);

        if (U) {
            ir.SetRegister(n, ir.Add(ir.GetRegister(n), index));
        } else {
            ir.SetRegister(n, ir.Sub(ir.GetRegister(n), index));
        }

        // TODO(bunnei): Handle W=1 mode, which in this scenario does an unprivileged (User mode) access.
    }
    return address;
}

bool ArmTranslatorVisitor::arm_LDR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm12 imm12) {
    if (ConditionPassed(cond)) {
        const auto data = ir.ReadMemory32(GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm12)));

        if (d == Reg::PC) {
            ir.BXWritePC(data);
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(d, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
    if (ConditionPassed(cond)) {
        const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
        const auto data = ir.ReadMemory32(GetAddressingMode(ir, P, U, W, n, shifted.result));

        if (d == Reg::PC) {
            ir.BXWritePC(data);
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(d, data);
    }

    return true;
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
    if (ConditionPassed(cond)) {
        const auto address = GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm12));
        ir.WriteMemory32(address, ir.GetRegister(d));
    }

    return true;
}

bool ArmTranslatorVisitor::arm_STR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
    if (ConditionPassed(cond)) {
        const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
        const auto address = GetAddressingMode(ir, P, U, W, n, shifted.result);
        ir.WriteMemory32(address, ir.GetRegister(d));
    }

    return true;
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
