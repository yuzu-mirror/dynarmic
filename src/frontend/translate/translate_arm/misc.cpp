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
