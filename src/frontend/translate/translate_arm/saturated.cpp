/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

bool ArmTranslatorVisitor::arm_QADD(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    // QADD <Rd>, <Rm>, <Rn>
    if (ConditionPassed(cond)) {
        auto a = ir.GetRegister(m);
        auto b = ir.GetRegister(n);
        auto result = ir.SignedSaturatedAdd(a, b);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_QSUB(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    // QSUB <Rd>, <Rm>, <Rn>
    if (ConditionPassed(cond)) {
        auto a = ir.GetRegister(m);
        auto b = ir.GetRegister(n);
        auto result = ir.SignedSaturatedSub(a, b);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_QDADD(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    // QDADD <Rd>, <Rm>, <Rn>
    if (ConditionPassed(cond)) {
        auto a = ir.GetRegister(m);
        auto b = ir.GetRegister(n);
        auto doubled = ir.SignedSaturatedAdd(b, b);
        ir.OrQFlag(doubled.overflow);
        auto result = ir.SignedSaturatedAdd(a, doubled.result);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_QDSUB(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    // QDSUB <Rd>, <Rm>, <Rn>
    if (ConditionPassed(cond)) {
        auto a = ir.GetRegister(m);
        auto b = ir.GetRegister(n);
        auto doubled = ir.SignedSaturatedAdd(b, b);
        ir.OrQFlag(doubled.overflow);
        auto result = ir.SignedSaturatedSub(a, doubled.result);
        ir.SetRegister(d, result.result);
        ir.OrQFlag(result.overflow);
    }
    return true;
}


} // namespace Arm
} // namespace Dynarmic
