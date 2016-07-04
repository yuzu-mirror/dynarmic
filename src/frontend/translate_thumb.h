/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "frontend/arm_types.h"
#include "frontend/ir_emitter.h"

namespace Dynarmic {
namespace Arm {

class TranslatorVisitor {
public:
    IREmitter ir;

    void thumb1_LSL_imm(Imm5 imm5, Reg m, Reg d) {
        u8 shift_n = imm5;
        // LSLS <Rd>, <Rm>, #<imm5>
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_n), cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    void thumb1_LSR_imm(Imm5 imm5, Reg m, Reg d) {
        u8 shift_n = imm5 != 0 ? imm5 : 32;
        // LSRS <Rd>, <Rm>, #<imm5>
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.LogicalShiftRight(ir.GetRegister(m), ir.Imm8(shift_n), cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    void thumb1_ASR_imm(Imm5 imm5, Reg m, Reg d) {
        u8 shift_n = imm5 != 0 ? imm5 : 32;
        // ASRS <Rd>, <Rm>, #<imm5>
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.ArithmeticShiftRight(ir.GetRegister(m), ir.Imm8(shift_n), cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    void thumb1_LSL_reg(Reg m, Reg d_n) {
        const Reg d = d_n, n = d_n;
        // LSLS <Rdn>, <Rm>
        auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
        auto apsr_c = ir.GetCFlag();
        auto result_carry = ir.LogicalShiftLeft(ir.GetRegister(n), shift_n, apsr_c);
        ir.SetRegister(d, result_carry.result);
        ir.SetNFlag(ir.MostSignificantBit(result_carry.result));
        ir.SetZFlag(ir.IsZero(result_carry.result));
        ir.SetCFlag(result_carry.carry);
    }
    void thumb1_LSR_reg(Reg m, Reg d_n) {
        const Reg d = d_n, n = d_n;
        // LSRS <Rdn>, <Rm>
        auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.LogicalShiftRight(ir.GetRegister(n), shift_n, cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }
    void thumb1_ASR_reg(Reg m, Reg d_n) {
        const Reg d = d_n, n = d_n;
        // ASRS <Rdn>, <Rm>
        auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.ArithmeticShiftRight(ir.GetRegister(n), shift_n, cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
    }


    void thumb1_UDF() {}
};

} // namespace Arm
} // namepsace Dynarmic
