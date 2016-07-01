/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "frontend_arm/arm_types.h"
#include "frontend_arm/ir_emitter.h"

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
    void thumb1_ASR_rri(Imm5 imm5, Reg m, Reg d) {
        ir.Unimplemented();
    }
    void thumb1_ADD_rrr(Reg m, Reg n, Reg d) {
        ir.Unimplemented();
    }
    void thumb1_SUB_rrr(Reg m, Reg n, Reg d) {
        ir.Unimplemented();
    }
    void thumb1_ADD_rri() {
        ir.Unimplemented();
    }
    void thumb1_SUB_rri() {
        ir.Unimplemented();
    }
    void thumb1_MOV_ri() {
        ir.Unimplemented();
    }
    void thumb1_CMP_ri() {
        ir.Unimplemented();
    }
    void thumb1_ADD_ri() {
        ir.Unimplemented();
    }
    void thumb1_SUB_ri() {
        ir.Unimplemented();
    }
    void thumb1_ANDS_rr() {
        ir.Unimplemented();
    }
    void thumb1_EORS_rr() {
        ir.Unimplemented();
    }
    void thumb1_LSLS_reg(Reg m, Reg d_n) {
        const Reg d = d_n, n = d_n;
        // LSLS <Rdn>, <Rm>
        auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
        auto apsr_c = ir.GetCFlag();
        auto result_carry = ir.LogicalShiftLeft(ir.GetRegister(d), shift_n, apsr_c);
        ir.SetRegister(d, result_carry.result);
        ir.SetNFlag(ir.MostSignificantBit(result_carry.result));
        ir.SetZFlag(ir.IsZero(result_carry.result));
        ir.SetCFlag(result_carry.carry);
    }
    void thumb1_LSRS_rr() {
        ir.Unimplemented();
    }
    void thumb1_ASRS_rr() {
        ir.Unimplemented();
    }
    void thumb1_ADCS_rr() {
        ir.Unimplemented();
    }
    void thumb1_SBCS_rr() {
        ir.Unimplemented();
    }
    void thumb1_RORS_rr() {
        ir.Unimplemented();
    }
    void thumb1_TST_rr() {
        ir.Unimplemented();
    }
    void thumb1_NEGS_rr() {
        ir.Unimplemented();
    }
    void thumb1_CMP_rr() {
        ir.Unimplemented();
    }
    void thumb1_CMN_rr() {
        ir.Unimplemented();
    }
    void thumb1_ORRS_rr() {
        ir.Unimplemented();
    }
    void thumb1_MULS_rr() {
        ir.Unimplemented();
    }
    void thumb1_BICS_rr() {
        ir.Unimplemented();
    }
    void thumb1_MVNS_rr() {
        ir.Unimplemented();
    }
    void thumb1_ADD_high() {
        ir.Unimplemented();
    }
    void thumb1_CMP_high() {
        ir.Unimplemented();
    }
    void thumb1_MOV_high() {
        ir.Unimplemented();
    }
    void thumb1_LDR_lit() {
        ir.Unimplemented();
    }
    void thumb1_STR_rrr() {
        ir.Unimplemented();
    }
    void thumb1_STRH_rrr() {
        ir.Unimplemented();
    }
    void thumb1_STRB_rrr() {
        ir.Unimplemented();
    }
    void thumb1_LDRSB_rrr() {
        ir.Unimplemented();
    }
    void thumb1_LDR_rrr() {
        ir.Unimplemented();
    }
    void thumb1_LDRH_rrr() {
        ir.Unimplemented();
    }
    void thumb1_LDRB_rrr() {
        ir.Unimplemented();
    }
    void thumb1_LDRSH_rrr() {
        ir.Unimplemented();
    }
    void thumb1_STRH_rri() {
        ir.Unimplemented();
    }
    void thumb1_LDRH_rri() {
        ir.Unimplemented();
    }
    void thumb1_STR_sp() {
        ir.Unimplemented();
    }
    void thumb1_LDR_sp() {
        ir.Unimplemented();
    }
    void thumb1_ADR() {
        ir.Unimplemented();
    }
    void thumb1_ADD_sp() {
        ir.Unimplemented();
    }
    void thumb1_ADD_spsp() {
        ir.Unimplemented();
    }
    void thumb1_SUB_spsp() {
        ir.Unimplemented();
    }
    void thumb1_SXTH() {
        ir.Unimplemented();
    }
    void thumb1_SXTB() {
        ir.Unimplemented();
    }
    void thumb1_UXTH() {
        ir.Unimplemented();
    }
    void thumb1_UXTB() {
        ir.Unimplemented();
    }
    void thumb1_PUSH() {
        ir.Unimplemented();
    }
    void thumb1_POP() {
        ir.Unimplemented();
    }
    void thumb1_SETEND() {
        ir.Unimplemented();
    }
    void thumb1_CPS() {
        ir.Unimplemented();
    }
    void thumb1_REV() {
        ir.Unimplemented();
    }
    void thumb1_REV16() {
        ir.Unimplemented();
    }
    void thumb1_REVSH() {
        ir.Unimplemented();
    }
    void thumb1_BKPT() {
        ir.Unimplemented();
    }
    void thumb1_STMIA() {
        ir.Unimplemented();
    }
    void thumb1_LDMIA() {
        ir.Unimplemented();
    }
    void thumb1_BX() {
        ir.Unimplemented();
    }
    void thumb1_BLX() {
        ir.Unimplemented();
    }
    void thumb1_BEQ() {
        ir.Unimplemented();
    }
    void thumb1_BNE() {
        ir.Unimplemented();
    }
    void thumb1_BCS() {
        ir.Unimplemented();
    }
    void thumb1_BCC() {
        ir.Unimplemented();
    }
    void thumb1_BMI() {
        ir.Unimplemented();
    }
    void thumb1_BPL() {
        ir.Unimplemented();
    }
    void thumb1_BVS() {
        ir.Unimplemented();
    }
    void thumb1_BVC() {
        ir.Unimplemented();
    }
    void thumb1_BHI() {
        ir.Unimplemented();
    }
    void thumb1_BLS() {
        ir.Unimplemented();
    }
    void thumb1_BGE() {
        ir.Unimplemented();
    }
    void thumb1_BLT() {
        ir.Unimplemented();
    }
    void thumb1_BGT() {
        ir.Unimplemented();
    }
    void thumb1_BLE() {
        ir.Unimplemented();
    }
    void thumb1_UDF() {
        ir.Unimplemented();
    }
    void thumb1_SWI() {
        ir.Unimplemented();
    }
    void thumb1_B() {
        ir.Unimplemented();
    }
    void thumb1_BLX_suffix() {
        ir.Unimplemented();
    }
    void thumb1_BLX_prefix() {
        ir.Unimplemented();
    }
    void thumb1_BL_suffix() {
        ir.Unimplemented();
    }
};

} // namespace Arm
} // namepsace Dynarmic
