/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

bool ArmTranslatorVisitor::arm_CLZ(Cond cond, Reg d, Reg m) {
    if (d == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        ir.SetRegister(d, ir.CountLeadingZeros(ir.GetRegister(m)));
    }
    return true;
}

} // namespace Arm
} // namespace Dynarmic
