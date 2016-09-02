/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

bool ArmTranslatorVisitor::arm_LDRBT() {
    ASSERT_MSG(false, "System instructions unimplemented");
}

bool ArmTranslatorVisitor::arm_LDRHT() {
    ASSERT_MSG(false, "System instructions unimplemented");
}

bool ArmTranslatorVisitor::arm_LDRSBT() {
    ASSERT_MSG(false, "System instructions unimplemented");
}

bool ArmTranslatorVisitor::arm_LDRSHT() {
    ASSERT_MSG(false, "System instructions unimplemented");
}

bool ArmTranslatorVisitor::arm_LDRT() {
    ASSERT_MSG(false, "System instructions unimplemented");
}

bool ArmTranslatorVisitor::arm_STRBT() {
    ASSERT_MSG(false, "System instructions unimplemented");
}

bool ArmTranslatorVisitor::arm_STRHT() {
    ASSERT_MSG(false, "System instructions unimplemented");
}

bool ArmTranslatorVisitor::arm_STRT() {
    ASSERT_MSG(false, "System instructions unimplemented");
}

static IR::Value GetAddressingMode(IR::IREmitter& ir, bool P, bool U, bool W, Reg n, IR::Value index) {
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

bool ArmTranslatorVisitor::arm_LDR_lit(Cond cond, bool U, Reg t, Imm12 imm12) {
    bool P = true, W = false;

    if (ConditionPassed(cond)) {
        const auto data = ir.ReadMemory32(GetAddressingMode(ir, P, U, W, Reg::PC, ir.Imm32(imm12)));

        if (t == Reg::PC) {
            ir.BXWritePC(data);
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm12 imm12) {
    if (n == Reg::PC)
        return UnpredictableInstruction();
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t)
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto data = ir.ReadMemory32(GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm12)));

        if (t == Reg::PC) {
            ir.BXWritePC(data);
            if (!P && W && n == Reg::R13)
                ir.SetTerm(IR::Term::PopRSBHint{});
            else
                ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm5 imm5, ShiftType shift, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (m == Reg::PC)
        return UnpredictableInstruction();
    if ((!P || W) && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
        const auto data = ir.ReadMemory32(GetAddressingMode(ir, P, U, W, n, shifted.result));

        if (t == Reg::PC) {
            ir.BXWritePC(data);
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRB_lit(Cond cond, bool U, Reg t, Imm12 imm12) {
    if (t == Reg::PC)
        return UnpredictableInstruction();

    bool P = true, W = false;
    if (ConditionPassed(cond)) {
        const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(GetAddressingMode(ir, P, U, W, Reg::PC, ir.Imm32(imm12))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm12 imm12) {
    if (n == Reg::PC)
        return UnpredictableInstruction();
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t)
        return UnpredictableInstruction();
    if (t == Reg::PC)
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm12))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm5 imm5, ShiftType shift, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if ((!P || W) && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
        const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(GetAddressingMode(ir, P, U, W, n, shifted.result)));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRD_lit(Cond cond, bool U, Reg t, Imm4 imm8a, Imm4 imm8b) {
    if (RegNumber(t) % 2 == 1)
        return UnpredictableInstruction();
    if (t+1 == Reg::PC)
        return UnpredictableInstruction();

    bool P = true, W = false;
    if (ConditionPassed(cond)) {
        const auto address_a = GetAddressingMode(ir, P, U, W, Reg::PC, ir.Imm32(imm8a << 4 | imm8b));
        const auto address_b = ir.Add(address_a, ir.Imm32(4));
        auto data_a = ir.ReadMemory32(address_a);
        auto data_b = ir.ReadMemory32(address_b);

        switch (t) {
        case Reg::PC:
            data_a = ir.Add(data_a, ir.Imm32(4));
            break;
        case Reg::LR:
            data_b = ir.Add(data_b, ir.Imm32(4));
            break;
        default:
            break;
        }

        if (t == Reg::PC) {
            ir.ALUWritePC(data_a);
        } else {
            ir.SetRegister(t, data_a);
        }

        const Reg reg_b = static_cast<Reg>(std::min(t + 1, Reg::R15));
        if (reg_b == Reg::PC) {
            ir.ALUWritePC(data_b);
        } else {
            ir.SetRegister(reg_b, data_b);
        }

        if (t == Reg::PC || reg_b == Reg::PC) {
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
    if (n == Reg::PC)
        return UnpredictableInstruction();
    if (RegNumber(t) % 2 == 1)
        return UnpredictableInstruction();
    if (!P && W)
        return UnpredictableInstruction();
    if ((!P || W) && (n == t || n == t+1))
        return UnpredictableInstruction();
    if (t+1 == Reg::PC)
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto address_a = GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm8a << 4 | imm8b));
        const auto address_b = ir.Add(address_a, ir.Imm32(4));
        auto data_a = ir.ReadMemory32(address_a);
        auto data_b = ir.ReadMemory32(address_b);

        switch (t) {
        case Reg::PC:
            data_a = ir.Add(data_a, ir.Imm32(4));
            break;
        case Reg::LR:
            data_b = ir.Add(data_b, ir.Imm32(4));
            break;
        default:
            break;
        }

        if (t == Reg::PC) {
            ir.ALUWritePC(data_a);
        } else {
            ir.SetRegister(t, data_a);
        }

        const Reg reg_b = static_cast<Reg>(std::min(t + 1, Reg::R15));
        if (reg_b == Reg::PC) {
            ir.ALUWritePC(data_b);
        } else {
            ir.SetRegister(reg_b, data_b);
        }

        if (t == Reg::PC || reg_b == Reg::PC) {
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    if (RegNumber(t) % 2 == 1)
        return UnpredictableInstruction();
    if (!P && W)
        return UnpredictableInstruction();
    if (t+1 == Reg::PC || m == Reg::PC || m == t || m == t+1)
        return UnpredictableInstruction();
    if ((!P || W) && (n == Reg::PC || n == t || n == t+1))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto address_a = GetAddressingMode(ir, P, U, W, n, ir.GetRegister(m));
        const auto address_b = ir.Add(address_a, ir.Imm32(4));
        auto data_a = ir.ReadMemory32(address_a);
        auto data_b = ir.ReadMemory32(address_b);

        switch (t) {
        case Reg::PC:
            data_a = ir.Add(data_a, ir.Imm32(4));
            break;
        case Reg::LR:
            data_b = ir.Add(data_b, ir.Imm32(4));
            break;
        default:
            break;
        }

        if (t == Reg::PC) {
            ir.ALUWritePC(data_a);
        } else {
            ir.SetRegister(t, data_a);
        }

        const Reg reg_b = static_cast<Reg>(std::min(t + 1, Reg::R15));
        if (reg_b == Reg::PC) {
            ir.ALUWritePC(data_b);
        } else {
            ir.SetRegister(reg_b, data_b);
        }

        if (t == Reg::PC || reg_b == Reg::PC) {
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRH_lit(Cond cond, bool P, bool U, bool W, Reg t, Imm4 imm8a, Imm4 imm8b) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (P == W)
        return UnpredictableInstruction();
    if (t == Reg::PC)
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(GetAddressingMode(ir, P, U, W, Reg::PC, ir.Imm32(imm8a << 4 | imm8b))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
    if (n == Reg::PC)
        return UnpredictableInstruction();
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t)
        return UnpredictableInstruction();
    if (t == Reg::PC)
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm8a << 4 | imm8b))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if ((!P || W) && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(GetAddressingMode(ir, P, U, W, n, ir.GetRegister(m))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRSB_lit(Cond cond, bool U, Reg t, Imm4 imm8a, Imm4 imm8b) {
    if (t == Reg::PC)
        return UnpredictableInstruction();

    bool P = true, W = false;
    if (ConditionPassed(cond)) {
        const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(GetAddressingMode(ir, P, U, W, Reg::PC, ir.Imm32(imm8a << 4 | imm8b))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRSB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
    if (n == Reg::PC)
        return UnpredictableInstruction();
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t)
        return UnpredictableInstruction();
    if (t == Reg::PC)
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm8a << 4 | imm8b))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRSB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if ((!P || W) && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(GetAddressingMode(ir, P, U, W, n, ir.GetRegister(m))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRSH_lit(Cond cond, bool U, Reg t, Imm4 imm8a, Imm4 imm8b) {
    if (t == Reg::PC)
        return UnpredictableInstruction();

    bool P = true, W = false;
    if (ConditionPassed(cond)) {
        const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(GetAddressingMode(ir, P, U, W, Reg::PC, ir.Imm32(imm8a << 4 | imm8b))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRSH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
    if (n == Reg::PC)
        return UnpredictableInstruction();
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t)
        return UnpredictableInstruction();
    if (t == Reg::PC)
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm8a << 4 | imm8b))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_LDRSH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();
    if ((!P || W) && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(GetAddressingMode(ir, P, U, W, n, ir.GetRegister(m))));

        if (t == Reg::PC) {
            ir.ALUWritePC(ir.Add(data, ir.Imm32(4)));
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        }

        ir.SetRegister(t, data);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_STR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm12 imm12) {
    if (W && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto address = GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm12));
        ir.WriteMemory32(address, ir.GetRegister(t));
    }

    return true;
}

bool ArmTranslatorVisitor::arm_STR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm5 imm5, ShiftType shift, Reg m) {
    if (m == Reg::PC)
        return UnpredictableInstruction();

    if (W && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
        const auto address = GetAddressingMode(ir, P, U, W, n, shifted.result);
        ir.WriteMemory32(address, ir.GetRegister(t));
    }

    return true;
}

bool ArmTranslatorVisitor::arm_STRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm12 imm12) {
    if (t == Reg::PC)
        return UnpredictableInstruction();

    if (W && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto address = GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm12));
        const auto value = (t == Reg::PC) ? ir.Imm8(ir.PC() - 8) : ir.GetRegister(t);
        ir.WriteMemory8(address, ir.LeastSignificantByte(value));
    }

    return true;
}

bool ArmTranslatorVisitor::arm_STRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm5 imm5, ShiftType shift, Reg m) {
    if (t == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    if (W && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
        const auto address = GetAddressingMode(ir, P, U, W, n, shifted.result);
        const auto value = (t == Reg::PC) ? ir.Imm8(ir.PC() - 8) : ir.GetRegister(t);
        ir.WriteMemory8(address, ir.LeastSignificantByte(value));
    }

    return true;
}

bool ArmTranslatorVisitor::arm_STRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
    if (size_t(t) % 2 != 0)
        return UnpredictableInstruction();

    if (!P && W)
        return UnpredictableInstruction();

    const Reg t2 = t + 1;

    if (W && (n == Reg::PC || n == t || n == t2))
        return UnpredictableInstruction();

    if (t2 == Reg::PC)
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto address_a = GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm8a << 4 | imm8b));
        const auto address_b = ir.Add(address_a, ir.Imm32(4));
        const auto value_a = ir.GetRegister(t);
        const auto value_b = ir.GetRegister(t2);
        ir.WriteMemory32(address_a, value_a);
        ir.WriteMemory32(address_b, value_b);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_STRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    if (size_t(t) % 2 != 0)
        return UnpredictableInstruction();

    if (!P && W)
        return UnpredictableInstruction();

    const Reg t2 = t + 1;

    if (t2 == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    if (W && (n == Reg::PC || n == t || n == t2))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto address_a = GetAddressingMode(ir, P, U, W, n, ir.GetRegister(m));
        const auto address_b = ir.Add(address_a, ir.Imm32(4));
        const auto value_a = ir.GetRegister(t);
        const auto value_b = ir.GetRegister(t2);
        ir.WriteMemory32(address_a, value_a);
        ir.WriteMemory32(address_b, value_b);
    }

    return true;
}

bool ArmTranslatorVisitor::arm_STRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
    if (t == Reg::PC)
        return UnpredictableInstruction();

    if (W && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto address = GetAddressingMode(ir, P, U, W, n, ir.Imm32(imm8a << 4 | imm8b));
        const auto value = (t == Reg::PC) ? ir.Imm32(ir.PC() - 8) : ir.GetRegister(t);
        ir.WriteMemory16(address, ir.LeastSignificantHalf(value));
    }

    return true;
}

bool ArmTranslatorVisitor::arm_STRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    if (t == Reg::PC || m == Reg::PC)
        return UnpredictableInstruction();

    if (W && (n == Reg::PC || n == t))
        return UnpredictableInstruction();

    if (ConditionPassed(cond)) {
        const auto address = GetAddressingMode(ir, P, U, W, n, ir.GetRegister(m));
        const auto value = (t == Reg::PC) ? ir.Imm32(ir.PC() - 8) : ir.GetRegister(t);
        ir.WriteMemory16(address, ir.LeastSignificantHalf(value));
    }

    return true;
}

static bool LDMHelper(IR::IREmitter& ir, bool W, Reg n, RegList list, IR::Value start_address, IR::Value writeback_address) {
    auto address = start_address;
    for (size_t i = 0; i <= 14; i++) {
        if (Common::Bit(i, list)) {
            ir.SetRegister(static_cast<Reg>(i), ir.ReadMemory32(address));
            address = ir.Add(address, ir.Imm32(4));
        }
    }
    if (W && !Common::Bit(RegNumber(n), list)) {
        ir.SetRegister(n, writeback_address);
    }
    if (Common::Bit<15>(list)) {
        ir.LoadWritePC(ir.ReadMemory32(address));
        if (n == Reg::R13)
            ir.SetTerm(IR::Term::PopRSBHint{});
        else
            ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }
    return true;
}

bool ArmTranslatorVisitor::arm_LDM(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1)
        return UnpredictableInstruction();
    // LDM <Rn>{!}, <reg_list>
    if (ConditionPassed(cond)) {
        auto start_address = ir.GetRegister(n);
        auto writeback_address = ir.Add(start_address, ir.Imm32(u32(Common::BitCount(list) * 4)));
        return LDMHelper(ir, W, n, list, start_address, writeback_address);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_LDMDA(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1)
        return UnpredictableInstruction();
    // LDMDA <Rn>{!}, <reg_list>
    if (ConditionPassed(cond)) {
        auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list) - 4)));
        auto writeback_address = ir.Sub(start_address, ir.Imm32(4));
        return LDMHelper(ir, W, n, list, start_address, writeback_address);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_LDMDB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1)
        return UnpredictableInstruction();
    // LDMDB <Rn>{!}, <reg_list>
    if (ConditionPassed(cond)) {
        auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list))));
        auto writeback_address = start_address;
        return LDMHelper(ir, W, n, list, start_address, writeback_address);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_LDMIB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1)
        return UnpredictableInstruction();
    // LDMIB <Rn>{!}, <reg_list>
    if (ConditionPassed(cond)) {
        auto start_address = ir.Add(ir.GetRegister(n), ir.Imm32(4));
        auto writeback_address = ir.Add(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list))));
        return LDMHelper(ir, W, n, list, start_address, writeback_address);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_LDM_usr() {
    return InterpretThisInstruction();
}

bool ArmTranslatorVisitor::arm_LDM_eret() {
    return InterpretThisInstruction();
}

static bool STMHelper(IR::IREmitter& ir, bool W, Reg n, RegList list, IR::Value start_address, IR::Value writeback_address) {
    auto address = start_address;
    for (size_t i = 0; i <= 14; i++) {
        if (Common::Bit(i, list)) {
            ir.WriteMemory32(address, ir.GetRegister(static_cast<Reg>(i)));
            address = ir.Add(address, ir.Imm32(4));
        }
    }
    if (W) {
        ir.SetRegister(n, writeback_address);
    }
    if (Common::Bit<15>(list)) {
        ir.WriteMemory32(address, ir.Imm32(ir.PC()));
    }
    return true;
}

bool ArmTranslatorVisitor::arm_STM(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1)
        return UnpredictableInstruction();
    // STM <Rn>{!}, <reg_list>
    if (ConditionPassed(cond)) {
        auto start_address = ir.GetRegister(n);
        auto writeback_address = ir.Add(start_address, ir.Imm32(u32(Common::BitCount(list) * 4)));
        return STMHelper(ir, W, n, list, start_address, writeback_address);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_STMDA(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1)
        return UnpredictableInstruction();
    // STMDA <Rn>{!}, <reg_list>
    if (ConditionPassed(cond)) {
        auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list) - 4)));
        auto writeback_address = ir.Sub(start_address, ir.Imm32(4));
        return STMHelper(ir, W, n, list, start_address, writeback_address);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_STMDB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1)
        return UnpredictableInstruction();
    // STMDB <Rn>{!}, <reg_list>
    if (ConditionPassed(cond)) {
        auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list))));
        auto writeback_address = start_address;
        return STMHelper(ir, W, n, list, start_address, writeback_address);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_STMIB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || Common::BitCount(list) < 1)
        return UnpredictableInstruction();
    // STMIB <Rn>{!}, <reg_list>
    if (ConditionPassed(cond)) {
        auto start_address = ir.Add(ir.GetRegister(n), ir.Imm32(4));
        auto writeback_address = ir.Add(ir.GetRegister(n), ir.Imm32(u32(4 * Common::BitCount(list))));
        return STMHelper(ir, W, n, list, start_address, writeback_address);
    }
    return true;
}

bool ArmTranslatorVisitor::arm_STM_usr() {
    return InterpretThisInstruction();
}

} // namespace Arm
} // namespace Dynarmic
