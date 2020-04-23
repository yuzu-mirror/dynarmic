/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "common/assert.h"
#include "frontend/imm.h"
#include "frontend/A32/ir_emitter.h"
#include "frontend/A32/location_descriptor.h"
#include "frontend/A32/translate/translate.h"
#include "frontend/A32/types.h"

namespace Dynarmic::A32 {

enum class Exception;

struct ThumbTranslatorVisitor final {
    using instruction_return_type = bool;

    explicit ThumbTranslatorVisitor(IR::Block& block, LocationDescriptor descriptor, const TranslationOptions& options) : ir(block, descriptor), options(options) {
        ASSERT_MSG(descriptor.TFlag(), "The processor must be in Thumb mode");
    }

    A32::IREmitter ir;
    TranslationOptions options;

    bool InterpretThisInstruction();
    bool UnpredictableInstruction();
    bool UndefinedInstruction();
    bool RaiseException(Exception exception);

    // thumb16
    bool thumb16_LSL_imm(Imm<5> imm5, Reg m, Reg d);
    bool thumb16_LSR_imm(Imm<5> imm5, Reg m, Reg d);
    bool thumb16_ASR_imm(Imm<5> imm5, Reg m, Reg d);
    bool thumb16_ADD_reg_t1(Reg m, Reg n, Reg d);
    bool thumb16_SUB_reg(Reg m, Reg n, Reg d);
    bool thumb16_ADD_imm_t1(Imm<3> imm3, Reg n, Reg d);
    bool thumb16_SUB_imm_t1(Imm<3> imm3, Reg n, Reg d);
    bool thumb16_MOV_imm(Reg d, Imm<8> imm8);
    bool thumb16_CMP_imm(Reg n, Imm<8> imm8);
    bool thumb16_ADD_imm_t2(Reg d_n, Imm<8> imm8);
    bool thumb16_SUB_imm_t2(Reg d_n, Imm<8> imm8);
    bool thumb16_AND_reg(Reg m, Reg d_n);
    bool thumb16_EOR_reg(Reg m, Reg d_n);
    bool thumb16_LSL_reg(Reg m, Reg d_n);
    bool thumb16_LSR_reg(Reg m, Reg d_n);
    bool thumb16_ASR_reg(Reg m, Reg d_n);
    bool thumb16_ADC_reg(Reg m, Reg d_n);
    bool thumb16_SBC_reg(Reg m, Reg d_n);
    bool thumb16_ROR_reg(Reg m, Reg d_n);
    bool thumb16_TST_reg(Reg m, Reg n);
    bool thumb16_RSB_imm(Reg n, Reg d);
    bool thumb16_CMP_reg_t1(Reg m, Reg n);
    bool thumb16_CMN_reg(Reg m, Reg n);
    bool thumb16_ORR_reg(Reg m, Reg d_n);
    bool thumb16_MUL_reg(Reg n, Reg d_m);
    bool thumb16_BIC_reg(Reg m, Reg d_n);
    bool thumb16_MVN_reg(Reg m, Reg d);
    bool thumb16_ADD_reg_t2(bool d_n_hi, Reg m, Reg d_n_lo);
    bool thumb16_CMP_reg_t2(bool n_hi, Reg m, Reg n_lo);
    bool thumb16_MOV_reg(bool d_hi, Reg m, Reg d_lo);
    bool thumb16_LDR_literal(Reg t, Imm<8> imm8);
    bool thumb16_STR_reg(Reg m, Reg n, Reg t);
    bool thumb16_STRH_reg(Reg m, Reg n, Reg t);
    bool thumb16_STRB_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRSB_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDR_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRH_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRB_reg(Reg m, Reg n, Reg t);
    bool thumb16_LDRSH_reg(Reg m, Reg n, Reg t);
    bool thumb16_STR_imm_t1(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_LDR_imm_t1(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_STRB_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_LDRB_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_STRH_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_LDRH_imm(Imm<5> imm5, Reg n, Reg t);
    bool thumb16_STR_imm_t2(Reg t, Imm<8> imm8);
    bool thumb16_LDR_imm_t2(Reg t, Imm<8> imm8);
    bool thumb16_ADR(Reg d, Imm<8> imm8);
    bool thumb16_ADD_sp_t1(Reg d, Imm<8> imm8);
    bool thumb16_ADD_sp_t2(Imm<7> imm7);
    bool thumb16_SUB_sp(Imm<7> imm7);
    bool thumb16_NOP();
    bool thumb16_SEV();
    bool thumb16_SEVL();
    bool thumb16_WFE();
    bool thumb16_WFI();
    bool thumb16_YIELD();
    bool thumb16_SXTH(Reg m, Reg d);
    bool thumb16_SXTB(Reg m, Reg d);
    bool thumb16_UXTH(Reg m, Reg d);
    bool thumb16_UXTB(Reg m, Reg d);
    bool thumb16_PUSH(bool M, RegList reg_list);
    bool thumb16_POP(bool P, RegList reg_list);
    bool thumb16_SETEND(bool E);
    bool thumb16_CPS(bool, bool, bool, bool);
    bool thumb16_REV(Reg m, Reg d);
    bool thumb16_REV16(Reg m, Reg d);
    bool thumb16_REVSH(Reg m, Reg d);
    bool thumb16_BKPT([[maybe_unused]] Imm<8> imm8);
    bool thumb16_STMIA(Reg n, RegList reg_list);
    bool thumb16_LDMIA(Reg n, RegList reg_list);
    bool thumb16_CBZ_CBNZ(bool nonzero, Imm<1> i, Imm<5> imm5, Reg n);
    bool thumb16_UDF();
    bool thumb16_BX(Reg m);
    bool thumb16_BLX_reg(Reg m);
    bool thumb16_SVC(Imm<8> imm8);
    bool thumb16_B_t1(Cond cond, Imm<8> imm8);
    bool thumb16_B_t2(Imm<11> imm11);

    // thumb32
    bool thumb32_BL_imm(Imm<11> hi, Imm<11> lo);
    bool thumb32_BLX_imm(Imm<11> hi, Imm<11> lo);
    bool thumb32_UDF();
};

} // namespace Dynarmic::A32
