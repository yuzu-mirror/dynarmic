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
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SADD16(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SASX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SSAX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SSUB8(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SSUB16(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedAddU8(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result.result);
        ir.SetGEFlags(result.ge);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UADD16(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UASX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_USAX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_USUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedSubU8(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result.result);
        ir.SetGEFlags(result.ge);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_USUB16(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}


// Parallel Add/Subtract (Saturating) instructions
bool ArmTranslatorVisitor::arm_QADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedSaturatedAddS8(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_QADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedSaturatedAddS16(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_QASX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_QSAX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_QSUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedSaturatedSubS8(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_QSUB16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedSaturatedSubS16(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UQADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (ConditionPassed(cond)) {
        auto result = ir.PackedSaturatedAddU8(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UQADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (ConditionPassed(cond)) {
        auto result = ir.PackedSaturatedAddU16(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UQASX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UQSAX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UQSUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedSaturatedSubU8(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UQSUB16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedSaturatedSubU16(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}


// Parallel Add/Subtract (Halving) instructions
bool ArmTranslatorVisitor::arm_SHADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedHalvingAddS8(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_SHADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedHalvingAddS16(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_SHASX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SHSAX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SHSUB8(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_SHSUB16(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UHADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedHalvingAddU8(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UHADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedHalvingAddU16(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UHASX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UHSAX(Cond cond, Reg n, Reg d, Reg m) {
    UNUSED(cond, n, d, m);
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_UHSUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedHalvingSubU8(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_UHSUB16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if (ConditionPassed(cond)) {
        auto result = ir.PackedHalvingSubU16(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
    }
    return true;
}

} // namespace Arm
} // namespace Dynarmic
