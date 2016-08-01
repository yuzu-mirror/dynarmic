/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

bool ArmTranslatorVisitor::arm_REV(Cond cond, Reg d, Reg m) {
    // REV<c> <Rd>, <Rm>
    ASSERT(d != Reg::PC && m != Reg::PC);

    if (ConditionPassed(cond)) {
        auto result = ir.ByteReverseWord(ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_REV16(Cond cond, Reg d, Reg m) {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_REVSH(Cond cond, Reg d, Reg m) {
    // REVSH<c> <Rd>, <Rm>
    ASSERT(d != Reg::PC && m != Reg::PC);

    if (ConditionPassed(cond)) {
        auto rev_half = ir.ByteReverseHalf(ir.LeastSignificantHalf(ir.GetRegister(m)));
        ir.SetRegister(d, ir.SignExtendHalfToWord(rev_half));
    }
    return true;
}

} // namespace Arm
} // namespace Dynarmic
