/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "frontend/ir/ir_emitter.h"

namespace Dynarmic {
namespace Arm {

enum class ConditionalState {
    /// We haven't met any conditional instructions yet.
    None,
    /// Current instruction is a conditional. This marks the end of this basic block.
    Break,
    /// This basic block is made up solely of conditional instructions.
    Translating,
};

struct ArmTranslatorVisitor final {
    explicit ArmTranslatorVisitor(LocationDescriptor descriptor) : ir(descriptor) {
        ASSERT_MSG(!descriptor.TFlag(), "The processor must be in Arm mode");
    }

    IREmitter ir;
    ConditionalState cond_state = ConditionalState::None;

    bool ConditionPassed(Cond cond);
    bool InterpretThisInstruction();
    bool UnpredictableInstruction();
    bool LinkToNextInstruction();

    static u32 rotr(u32 x, int shift) {
        shift &= 31;
        if (!shift) return x;
        return (x >> shift) | (x << (32 - shift));
    }

    static u32 ArmExpandImm(int rotate, Imm8 imm8) {
        return rotr(static_cast<u32>(imm8), rotate*2);
    }

    // Data processing instructions
    bool arm_ADC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_ADC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_ADC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_ADD_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_ADD_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_ADD_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_AND_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_AND_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_AND_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_BIC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_BIC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_BIC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_CMN_imm(Cond cond, Reg n, int rotate, Imm8 imm8);
    bool arm_CMN_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_CMN_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m);
    bool arm_CMP_imm(Cond cond, Reg n, int rotate, Imm8 imm8);
    bool arm_CMP_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_CMP_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m);
    bool arm_EOR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_EOR_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_EOR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_MOV_imm(Cond cond, bool S, Reg d, int rotate, Imm8 imm8);
    bool arm_MOV_reg(Cond cond, bool S, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_MOV_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_MVN_imm(Cond cond, bool S, Reg d, int rotate, Imm8 imm8);
    bool arm_MVN_reg(Cond cond, bool S, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_MVN_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_ORR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_ORR_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_ORR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_RSB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_RSB_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_RSB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_RSC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_RSC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_RSC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_SBC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_SBC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_SBC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_SUB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8);
    bool arm_SUB_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_SUB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m);
    bool arm_TEQ_imm(Cond cond, Reg n, int rotate, Imm8 imm8);
    bool arm_TEQ_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_TEQ_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m);
    bool arm_TST_imm(Cond cond, Reg n, int rotate, Imm8 imm8);
    bool arm_TST_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m);
    bool arm_TST_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m);

    // Exception generating instructions
    bool arm_BKPT(Cond cond, Imm12 imm12, Imm4 imm4);
    bool arm_SVC(Cond cond, Imm24 imm24);
    bool arm_UDF();

    // Reversal instructions
    bool arm_REV(Cond cond, Reg d, Reg m);
    bool arm_REV16(Cond cond, Reg d, Reg m);
    bool arm_REVSH(Cond cond, Reg d, Reg m);

    // Floating-point three-register data processing instructions
    bool vfp2_VADD(Cond cond, bool D, bool sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm);
};

} // namespace Arm
} // namespace Dynarmic
