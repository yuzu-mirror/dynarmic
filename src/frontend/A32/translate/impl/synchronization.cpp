/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

// CLREX
bool ArmTranslatorVisitor::arm_CLREX() {
    ir.ClearExclusive();
    return true;
}

// SWP<c> <Rt>, <Rt2>, [<Rn>]
// TODO: UNDEFINED if current mode is Hypervisor
bool ArmTranslatorVisitor::arm_SWP(Cond cond, Reg n, Reg t, Reg t2) {
    if (t == Reg::PC || t2 == Reg::PC || n == Reg::PC || n == t || n == t2) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto data = ir.ReadMemory32(ir.GetRegister(n));
    ir.WriteMemory32(ir.GetRegister(n), ir.GetRegister(t2));
    // TODO: Alignment check
    ir.SetRegister(t, data);
    return true;
}

// SWPB<c> <Rt>, <Rt2>, [<Rn>]
// TODO: UNDEFINED if current mode is Hypervisor
bool ArmTranslatorVisitor::arm_SWPB(Cond cond, Reg n, Reg t, Reg t2) {
    if (t == Reg::PC || t2 == Reg::PC || n == Reg::PC || n == t || n == t2) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto data = ir.ReadMemory8(ir.GetRegister(n));
    ir.WriteMemory8(ir.GetRegister(n), ir.LeastSignificantByte(ir.GetRegister(t2)));
    // TODO: Alignment check
    ir.SetRegister(t, ir.ZeroExtendByteToWord(data));
    return true;
}

// LDA<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDA(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetRegister(t, ir.ReadMemory32(address)); // AccType::Ordered
    return true;
}
// LDAB<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDAB(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetRegister(t, ir.ZeroExtendToWord(ir.ReadMemory8(address))); // AccType::Ordered
    return true;
}
// LDAH<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDAH(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetRegister(t, ir.ZeroExtendToWord(ir.ReadMemory16(address))); // AccType::Ordered
    return true;
}

// LDAEX<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDAEX(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetExclusive(address, 4);
    ir.SetRegister(t, ir.ReadMemory32(address)); // AccType::Ordered
    return true;
}

// LDAEXB<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDAEXB(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetExclusive(address, 1);
    ir.SetRegister(t, ir.ZeroExtendByteToWord(ir.ReadMemory8(address))); // AccType::Ordered
    return true;
}

// LDAEXD<c> <Rt>, <Rt2>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDAEXD(Cond cond, Reg n, Reg t) {
    if (t == Reg::LR || t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetExclusive(address, 8);

    // DO NOT SWAP hi AND lo IN BIG ENDIAN MODE, THIS IS CORRECT BEHAVIOUR
    const auto lo = ir.ReadMemory32(address); // AccType::Ordered
    ir.SetRegister(t, lo);
    const auto hi = ir.ReadMemory32(ir.Add(address, ir.Imm32(4))); // AccType::Ordered
    ir.SetRegister(t+1, hi);
    return true;
}

// LDAEXH<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDAEXH(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetExclusive(address, 2);
    ir.SetRegister(t, ir.ZeroExtendHalfToWord(ir.ReadMemory16(address))); // AccType::Ordered
    return true;
}

// STL<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_STL(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.WriteMemory32(address, ir.GetRegister(t)); // AccType::Ordered
    return true;
}

// STLB<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_STLB(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.WriteMemory8(address, ir.LeastSignificantByte(ir.GetRegister(t))); // AccType::Ordered
    return true;
}

// STLH<c> <Rd>, <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_STLH(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.WriteMemory16(address, ir.LeastSignificantHalf(ir.GetRegister(t))); // AccType::Ordered
    return true;
}

// STLEXB<c> <Rd>, <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_STLEXB(Cond cond, Reg n, Reg d, Reg t) {
    if (n == Reg::PC || d == Reg::PC || t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.LeastSignificantByte(ir.GetRegister(t));
    const auto passed = ir.ExclusiveWriteMemory8(address, value); // AccType::Ordered
    ir.SetRegister(d, passed);
    return true;
}
// STLEXD<c> <Rd>, <Rt>, <Rt2>, [<Rn>]
bool ArmTranslatorVisitor::arm_STLEXD(Cond cond, Reg n, Reg d, Reg t) {
    if (n == Reg::PC || d == Reg::PC || t == Reg::LR || static_cast<size_t>(t) % 2 == 1) {
        return UnpredictableInstruction();
    }

    if (d == n || d == t || d == t+1) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const Reg t2 = t + 1;
    const auto address = ir.GetRegister(n);
    const auto value_lo = ir.GetRegister(t);
    const auto value_hi = ir.GetRegister(t2);
    const auto passed = ir.ExclusiveWriteMemory64(address, value_lo, value_hi); // AccType::Ordered
    ir.SetRegister(d, passed);
    return true;
}

// STLEXH<c> <Rd>, <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_STLEXH(Cond cond, Reg n, Reg d, Reg t) {
    if (n == Reg::PC || d == Reg::PC || t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.LeastSignificantHalf(ir.GetRegister(t));
    const auto passed = ir.ExclusiveWriteMemory16(address, value); // AccType::Ordered
    ir.SetRegister(d, passed);
    return true;
}

// STLEX<c> <Rd>, <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_STLEX(Cond cond, Reg n, Reg d, Reg t) {
    if (n == Reg::PC || d == Reg::PC || t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.GetRegister(t);
    const auto passed = ir.ExclusiveWriteMemory32(address, value);
    ir.SetRegister(d, passed);
    return true;
}

// LDREX<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDREX(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetExclusive(address, 4);
    ir.SetRegister(t, ir.ReadMemory32(address));
    return true;
}

// LDREXB<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDREXB(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetExclusive(address, 1);
    ir.SetRegister(t, ir.ZeroExtendByteToWord(ir.ReadMemory8(address)));
    return true;
}

// LDREXD<c> <Rt>, <Rt2>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDREXD(Cond cond, Reg n, Reg t) {
    if (t == Reg::LR || t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetExclusive(address, 8);

    // DO NOT SWAP hi AND lo IN BIG ENDIAN MODE, THIS IS CORRECT BEHAVIOUR
    const auto lo = ir.ReadMemory32(address);
    ir.SetRegister(t, lo);
    const auto hi = ir.ReadMemory32(ir.Add(address, ir.Imm32(4)));
    ir.SetRegister(t+1, hi);
    return true;
}

// LDREXH<c> <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_LDREXH(Cond cond, Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    ir.SetExclusive(address, 2);
    ir.SetRegister(t, ir.ZeroExtendHalfToWord(ir.ReadMemory16(address)));
    return true;
}

// STREX<c> <Rd>, <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_STREX(Cond cond, Reg n, Reg d, Reg t) {
    if (n == Reg::PC || d == Reg::PC || t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.GetRegister(t);
    const auto passed = ir.ExclusiveWriteMemory32(address, value);
    ir.SetRegister(d, passed);
    return true;
}

// STREXB<c> <Rd>, <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_STREXB(Cond cond, Reg n, Reg d, Reg t) {
    if (n == Reg::PC || d == Reg::PC || t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.LeastSignificantByte(ir.GetRegister(t));
    const auto passed = ir.ExclusiveWriteMemory8(address, value);
    ir.SetRegister(d, passed);
    return true;
}

// STREXD<c> <Rd>, <Rt>, <Rt2>, [<Rn>]
bool ArmTranslatorVisitor::arm_STREXD(Cond cond, Reg n, Reg d, Reg t) {
    if (n == Reg::PC || d == Reg::PC || t == Reg::LR || static_cast<size_t>(t) % 2 == 1) {
        return UnpredictableInstruction();
    }

    if (d == n || d == t || d == t+1) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const Reg t2 = t + 1;
    const auto address = ir.GetRegister(n);
    const auto value_lo = ir.GetRegister(t);
    const auto value_hi = ir.GetRegister(t2);
    const auto passed = ir.ExclusiveWriteMemory64(address, value_lo, value_hi);
    ir.SetRegister(d, passed);
    return true;
}

// STREXH<c> <Rd>, <Rt>, [<Rn>]
bool ArmTranslatorVisitor::arm_STREXH(Cond cond, Reg n, Reg d, Reg t) {
    if (n == Reg::PC || d == Reg::PC || t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    if (!ConditionPassed(cond)) {
        return true;
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.LeastSignificantHalf(ir.GetRegister(t));
    const auto passed = ir.ExclusiveWriteMemory16(address, value);
    ir.SetRegister(d, passed);
    return true;
}

} // namespace Dynarmic::A32
