/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/bit_util.h"

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

bool ArmTranslatorVisitor::arm_B(Cond cond, Imm24 imm24)
{
    s32 imm32 = Common::SignExtend<26, s32>(imm24 << 2) + 8;
    // B <cond> <label>
    if (ConditionPassed(cond))
    {
        auto new_location = ir.current_location.AdvancePC(imm32);
        ir.SetTerm(IR::Term::LinkBlock{ new_location });
        return false;
    }
    return true;
}

bool ArmTranslatorVisitor::arm_BL(Cond cond, Imm24 imm24)
{
    s32 imm32 = Common::SignExtend<26, s32>(imm24 << 2) + 8;
    // BL <cond> <label>
    if (ConditionPassed(cond))
    {
        ir.SetRegister(Reg::LR, ir.Imm32(ir.current_location.PC() + 4));
        auto new_location = ir.current_location.AdvancePC(imm32);
        ir.SetTerm(IR::Term::LinkBlock{ new_location });
        return false;
    }
    return true;
}

bool ArmTranslatorVisitor::arm_BLX_imm(Cond cond, Imm24 imm24)
{
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_BLX_reg(Cond cond, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_BX(Cond cond, Reg m) {
    // BX <cond> <Rm>
    if (ConditionPassed(cond)) {
        ir.BXWritePC(ir.GetRegister(m));
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }
    return true;
}

bool ArmTranslatorVisitor::arm_BXJ(Cond cond, Reg m) {
    return InterpretThisInstruction();
}

} // namespace Arm
} // namespace Dynarmic
