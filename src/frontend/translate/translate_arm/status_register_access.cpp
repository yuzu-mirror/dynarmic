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

bool ArmTranslatorVisitor::arm_MRS(Cond cond, Reg d) {
    if (d == Reg::PC)
        return UnpredictableInstruction();
    // MRS <Rd>, APSR
    if (ConditionPassed(cond)) {
        ir.SetRegister(d, ir.GetCpsr());
    }
    return true;
}

bool ArmTranslatorVisitor::arm_MSR_imm(Cond cond, int mask, int rotate, Imm8 imm8) {
    bool write_nzcvq = Common::Bit<1>(mask);
    bool write_g = Common::Bit<0>(mask);
    u32 imm32 = ArmExpandImm(rotate, imm8);
    ASSERT_MSG(write_nzcvq || write_g, "Decode error");
    // MSR <spec_reg>, #<imm32>
    if (ConditionPassed(cond)) {
        u32 cpsr_mask = 0;
        if (write_nzcvq)
            cpsr_mask |= 0xF8000000;
        if (write_g)
            cpsr_mask |= 0x000F0000;
        auto old_cpsr = ir.And(ir.GetCpsr(), ir.Imm32(~cpsr_mask));
        auto new_cpsr = ir.Imm32(imm32 & cpsr_mask);
        ir.SetCpsr(ir.Or(old_cpsr, new_cpsr));
    }
    return true;
}

bool ArmTranslatorVisitor::arm_MSR_reg(Cond cond, int mask, Reg n) {
    bool write_nzcvq = Common::Bit<1>(mask);
    bool write_g = Common::Bit<0>(mask);
    if (!write_nzcvq && !write_g)
        return UnpredictableInstruction();
    if (n == Reg::PC)
        return UnpredictableInstruction();
    // MSR <spec_reg>, #<imm32>
    if (ConditionPassed(cond)) {
        u32 cpsr_mask = 0;
        if (write_nzcvq)
            cpsr_mask |= 0xF8000000;
        if (write_g)
            cpsr_mask |= 0x000F0000;
        auto old_cpsr = ir.And(ir.GetCpsr(), ir.Imm32(~cpsr_mask));
        auto new_cpsr = ir.And(ir.GetRegister(n), ir.Imm32(cpsr_mask));
        ir.SetCpsr(ir.Or(old_cpsr, new_cpsr));
    }
    return true;
}

bool ArmTranslatorVisitor::arm_RFE() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SETEND(bool E) {
    // SETEND {BE,LE}
    ir.SetTerm(IR::Term::LinkBlock{ir.current_location.AdvancePC(4).SetEFlag(E)});
    return false;
}

bool ArmTranslatorVisitor::arm_SRS() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SEL(Cond cond, Reg n, Reg d, Reg m) {
    if (n == Reg::PC || d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        auto ge = ir.GetGEFlags();

        // Perform some arithmetic to expand 0bXYZW into 0bXXXXXXXXYYYYYYYYZZZZZZZZWWWWWWWW => 0xXXYYZZWW
        // The logic behind this is as follows:
        // 0000 0000 0000 0000 | 0000 0000 0000 xyzw
        // 0000 000x yzw0 00xy | zw00 0xyz w000 xyzw (x * 0x00204081)
        // 0000 000x 0000 000y | 0000 000z 0000 000w (x & 0x01010101)
        // xxxx xxxx yyyy yyyy | zzzz zzzz wwww wwww (x * 0xff)

        auto x2 = ir.Mul(ge, ir.Imm32(0x00204081));
        auto x3 = ir.And(x2, ir.Imm32(0x01010101));
        auto mask = ir.Mul(x3, ir.Imm32(0xFF));

        auto to = ir.GetRegister(m);
        auto from = ir.GetRegister(n);
        auto result = ir.Or(ir.And(from, mask), ir.And(to, ir.Not(mask)));
        ir.SetRegister(d, result);
    }

    return true;
}

} // namespace Arm
} // namespace Dynarmic
