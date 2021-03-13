/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "common/assert.h"
#include "frontend/imm.h"
#include "frontend/A32/ir_emitter.h"
#include "frontend/A32/location_descriptor.h"
#include "frontend/A32/translate/conditional_state.h"
#include "frontend/A32/translate/translate.h"
#include "frontend/A32/types.h"

namespace Dynarmic::A32 {

enum class Exception;

struct ThumbTranslatorVisitor final {
    using instruction_return_type = bool;

    explicit ThumbTranslatorVisitor(IR::Block& block, LocationDescriptor descriptor, const TranslationOptions& options) : ir(block, descriptor, options.arch_version), options(options) {
        ASSERT_MSG(descriptor.TFlag(), "The processor must be in Thumb mode");
    }

    struct ImmAndCarry {
        u32 imm32;
        IR::U1 carry;
    };

    ImmAndCarry ThumbExpandImm_C(Imm<1> i, Imm<3> imm3, Imm<8> imm8, IR::U1 carry_in) {
        const Imm<12> imm12 = concatenate(i, imm3, imm8);
        if (imm12.Bits<10, 11>() == 0) {
            const u32 imm32 = [&]{
                const u32 imm8 = imm12.Bits<0, 7>();
                switch (imm12.Bits<8, 9>()) {
                case 0b00:
                    return imm8;
                case 0b01:
                    return Common::Replicate(imm8, 16);
                case 0b10:
                    return Common::Replicate(imm8 << 8, 16);
                case 0b11:
                    return Common::Replicate(imm8, 8);
                }
                UNREACHABLE();
            }();
            return {imm32, carry_in};
        }
        const u32 imm32 = Common::RotateRight<u32>((1 << 7) | imm12.Bits<0, 6>(), imm12.Bits<7, 11>());
        return {imm32, ir.Imm1(Common::Bit<31>(imm32))};
    }

    u32 ThumbExpandImm(Imm<1> i, Imm<3> imm3, Imm<8> imm8) {
        return ThumbExpandImm_C(i, imm3, imm8, ir.Imm1(0)).imm32;
    }

    IR::ResultAndCarry<IR::U32> EmitImmShift(IR::U32 value, ShiftType type, Imm<3> imm3, Imm<2> imm2, IR::U1 carry_in) {
        u8 imm5_value = concatenate(imm3, imm2).ZeroExtend<u8>();
        switch (type) {
        case ShiftType::LSL:
            return ir.LogicalShiftLeft(value, ir.Imm8(imm5_value), carry_in);
        case ShiftType::LSR:
            imm5_value = imm5_value ? imm5_value : 32;
            return ir.LogicalShiftRight(value, ir.Imm8(imm5_value), carry_in);
        case ShiftType::ASR:
            imm5_value = imm5_value ? imm5_value : 32;
            return ir.ArithmeticShiftRight(value, ir.Imm8(imm5_value), carry_in);
        case ShiftType::ROR:
            if (imm5_value) {
                return ir.RotateRight(value, ir.Imm8(imm5_value), carry_in);
            } else {
                return ir.RotateRightExtended(value, carry_in);
            }
        }
        UNREACHABLE();
    }

    A32::IREmitter ir;
    ConditionalState cond_state = ConditionalState::None;
    TranslationOptions options;

    bool ConditionPassed(bool is_thumb_16);

    bool InterpretThisInstruction();
    bool UnpredictableInstruction();
    bool UndefinedInstruction();
    bool RaiseException(Exception exception);

    IR::ResultAndCarry<IR::U32> EmitImmShift(IR::U32 value, ShiftType type, Imm<5> imm5, IR::U1 carry_in);

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
    bool thumb16_SEV();
    bool thumb16_SEVL();
    bool thumb16_WFE();
    bool thumb16_WFI();
    bool thumb16_YIELD();
    bool thumb16_NOP();
    bool thumb16_IT(Imm<8> imm8);
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

    // thumb32 load/store multiple instructions
    bool thumb32_LDMDB(bool W, Reg n, Imm<16> reg_list);
    bool thumb32_LDMIA(bool W, Reg n, Imm<16> reg_list);
    bool thumb32_POP(Imm<16> reg_list);
    bool thumb32_PUSH(Imm<15> reg_list);
    bool thumb32_STMIA(bool W, Reg n, Imm<15> reg_list);
    bool thumb32_STMDB(bool W, Reg n, Imm<15> reg_list);

    // thumb32 data processing (shifted register) instructions
    bool thumb32_TST_reg(Reg n, Imm<3> imm3, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_AND_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_BIC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_MOV_reg(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_ORR_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_MVN_reg(bool S, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_ORN_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_TEQ_reg(Reg n, Imm<3> imm3, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_EOR_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_PKH(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<1> tb, Reg m);
    bool thumb32_CMN_reg(Reg n, Imm<3> imm3, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_ADD_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_ADC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_SBC_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_CMP_reg(Reg n, Imm<3> imm3, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_SUB_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);
    bool thumb32_RSB_reg(bool S, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, ShiftType type, Reg m);

    // thumb32 data processing (modified immediate) instructions
    bool thumb32_TST_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_AND_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_BIC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_MOV_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ORR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_MVN_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ORN_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_TEQ_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_EOR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_CMN_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_ADD_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ADC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_SBC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_CMP_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_SUB_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_RSB_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);

    // thumb32 data processing (plain binary immediate) instructions.
    bool thumb32_ADR_t2(Imm<1> imm1, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ADR_t3(Imm<1> imm1, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ADD_imm_2(Imm<1> imm1, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_BFC(Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> msb);
    bool thumb32_BFI(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> msb);
    bool thumb32_MOVT(Imm<1> imm1, Imm<4> imm4, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_MOVW_imm(Imm<1> imm1, Imm<4> imm4, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_SBFX(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> widthm1);
    bool thumb32_SSAT(bool sh, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> sat_imm);
    bool thumb32_SSAT16(Reg n, Reg d, Imm<4> sat_imm);
    bool thumb32_SUB_imm_2(Imm<1> imm1, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_UBFX(Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> widthm1);
    bool thumb32_USAT(bool sh, Reg n, Imm<3> imm3, Reg d, Imm<2> imm2, Imm<5> sat_imm);
    bool thumb32_USAT16(Reg n, Reg d, Imm<4> sat_imm);

    // thumb32 miscellaneous control instructions
    bool thumb32_BXJ(Reg m);
    bool thumb32_CLREX();
    bool thumb32_DMB(Imm<4> option);
    bool thumb32_DSB(Imm<4> option);
    bool thumb32_ISB(Imm<4> option);
    bool thumb32_NOP();
    bool thumb32_SEV();
    bool thumb32_SEVL();
    bool thumb32_UDF();
    bool thumb32_WFE();
    bool thumb32_WFI();
    bool thumb32_YIELD();

    // thumb32 branch instructions
    bool thumb32_BL_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);
    bool thumb32_BLX_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);
    bool thumb32_B(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);
    bool thumb32_B_cond(Imm<1> S, Cond cond, Imm<6> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);

    // thumb32 store single data item instructions
    bool thumb32_STRB_imm_1(Reg n, Reg t, bool P, bool U, Imm<8> imm8);
    bool thumb32_STRB_imm_2(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STRB_imm_3(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_STRBT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STRB(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_STRH_imm_1(Reg n, Reg t, bool P, bool U, Imm<8> imm8);
    bool thumb32_STRH_imm_2(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STRH_imm_3(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_STRHT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STRH(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_STR_imm_1(Reg n, Reg t, bool P, bool U, Imm<8> imm8);
    bool thumb32_STR_imm_2(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STR_imm_3(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_STRT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_STR_reg(Reg n, Reg t, Imm<2> imm2, Reg m);

    // thumb32 load byte and memory hints
    bool thumb32_PLD_lit(bool U, Imm<12> imm12);
    bool thumb32_PLD_imm8(bool W, Reg n, Imm<8> imm8);
    bool thumb32_PLD_imm12(bool W, Reg n, Imm<12> imm12);
    bool thumb32_PLD_reg(bool W, Reg n, Imm<2> imm2, Reg m);
    bool thumb32_PLI_lit(bool U, Imm<12> imm12);
    bool thumb32_PLI_imm8(Reg n, Imm<8> imm8);
    bool thumb32_PLI_imm12(Reg n, Imm<12> imm12);
    bool thumb32_PLI_reg(Reg n, Imm<2> imm2, Reg m);
    bool thumb32_LDRB_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDRB_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDRB_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDRB_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRBT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_LDRSB_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDRSB_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDRSB_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDRSB_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRSBT(Reg n, Reg t, Imm<8> imm8);

    // thumb32 load halfword instructions
    bool thumb32_LDRH_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDRH_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDRH_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDRH_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRHT(Reg n, Reg t, Imm<8> imm8);
    bool thumb32_LDRSH_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDRSH_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDRSH_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDRSH_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRSHT(Reg n, Reg t, Imm<8> imm8);

    // thumb32 load word instructions
    bool thumb32_LDR_lit(bool U, Reg t, Imm<12> imm12);
    bool thumb32_LDR_reg(Reg n, Reg t, Imm<2> imm2, Reg m);
    bool thumb32_LDR_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8);
    bool thumb32_LDR_imm12(Reg n, Reg t, Imm<12> imm12);
    bool thumb32_LDRT(Reg n, Reg t, Imm<8> imm8);

    // thumb32 data processing (register) instructions
    bool thumb32_ASR_reg(Reg m, Reg d, Reg s);
    bool thumb32_LSL_reg(Reg m, Reg d, Reg s);
    bool thumb32_LSR_reg(Reg m, Reg d, Reg s);
    bool thumb32_ROR_reg(Reg m, Reg d, Reg s);
    bool thumb32_SXTB(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTB16(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTAB(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTAB16(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTH(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_SXTAH(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTB(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTB16(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTAB(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTAB16(Reg n, Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTH(Reg d, SignExtendRotation rotate, Reg m);
    bool thumb32_UXTAH(Reg n, Reg d, SignExtendRotation rotate, Reg m);

    // thumb32 long multiply, long multiply accumulate, and divide instructions
    bool thumb32_SDIV(Reg n, Reg d, Reg m);
    bool thumb32_SMLAL(Reg n, Reg dLo, Reg dHi, Reg m);
    bool thumb32_SMLALD(Reg n, Reg dLo, Reg dHi, bool M, Reg m);
    bool thumb32_SMLALXY(Reg n, Reg dLo, Reg dHi, bool N, bool M, Reg m);
    bool thumb32_SMLSLD(Reg n, Reg dLo, Reg dHi, bool M, Reg m);
    bool thumb32_SMULL(Reg n, Reg dLo, Reg dHi, Reg m);
    bool thumb32_UDIV(Reg n, Reg d, Reg m);
    bool thumb32_UMAAL(Reg n, Reg dLo, Reg dHi, Reg m);
    bool thumb32_UMLAL(Reg n, Reg dLo, Reg dHi, Reg m);
    bool thumb32_UMULL(Reg n, Reg dLo, Reg dHi, Reg m);

    // thumb32 miscellaneous instructions
    bool thumb32_CLZ(Reg n, Reg d, Reg m);
    bool thumb32_QADD(Reg n, Reg d, Reg m);
    bool thumb32_QDADD(Reg n, Reg d, Reg m);
    bool thumb32_QDSUB(Reg n, Reg d, Reg m);
    bool thumb32_QSUB(Reg n, Reg d, Reg m);
    bool thumb32_RBIT(Reg n, Reg d, Reg m);
    bool thumb32_REV(Reg n, Reg d, Reg m);
    bool thumb32_REV16(Reg n, Reg d, Reg m);
    bool thumb32_REVSH(Reg n, Reg d, Reg m);
    bool thumb32_SEL(Reg n, Reg d, Reg m);

    // thumb32 multiply instructions
    bool thumb32_MLA(Reg n, Reg a, Reg d, Reg m);
    bool thumb32_MLS(Reg n, Reg a, Reg d, Reg m);
    bool thumb32_MUL(Reg n, Reg d, Reg m);
    bool thumb32_SMLAD(Reg n, Reg a, Reg d, bool X, Reg m);
    bool thumb32_SMLAXY(Reg n, Reg a, Reg d, bool N, bool M, Reg m);
    bool thumb32_SMLAWY(Reg n, Reg a, Reg d, bool M, Reg m);
    bool thumb32_SMLSD(Reg n, Reg a, Reg d, bool X, Reg m);
    bool thumb32_SMMLA(Reg n, Reg a, Reg d, bool R, Reg m);
    bool thumb32_SMMLS(Reg n, Reg a, Reg d, bool R, Reg m);
    bool thumb32_SMMUL(Reg n, Reg d, bool R, Reg m);
    bool thumb32_SMUAD(Reg n, Reg d, bool M, Reg m);
    bool thumb32_SMUSD(Reg n, Reg d, bool M, Reg m);
    bool thumb32_SMULXY(Reg n, Reg d, bool N, bool M, Reg m);
    bool thumb32_SMULWY(Reg n, Reg d, bool M, Reg m);
    bool thumb32_USAD8(Reg n, Reg d, Reg m);
    bool thumb32_USADA8(Reg n, Reg a, Reg d, Reg m);

    // thumb32 parallel add/sub instructions
    bool thumb32_SADD8(Reg n, Reg d, Reg m);
    bool thumb32_SADD16(Reg n, Reg d, Reg m);
    bool thumb32_SASX(Reg n, Reg d, Reg m);
    bool thumb32_SSAX(Reg n, Reg d, Reg m);
    bool thumb32_SSUB8(Reg n, Reg d, Reg m);
    bool thumb32_SSUB16(Reg n, Reg d, Reg m);
    bool thumb32_UADD8(Reg n, Reg d, Reg m);
    bool thumb32_UADD16(Reg n, Reg d, Reg m);
    bool thumb32_UASX(Reg n, Reg d, Reg m);
    bool thumb32_USAX(Reg n, Reg d, Reg m);
    bool thumb32_USUB8(Reg n, Reg d, Reg m);
    bool thumb32_USUB16(Reg n, Reg d, Reg m);

    bool thumb32_QADD8(Reg n, Reg d, Reg m);
    bool thumb32_QADD16(Reg n, Reg d, Reg m);
    bool thumb32_QASX(Reg n, Reg d, Reg m);
    bool thumb32_QSAX(Reg n, Reg d, Reg m);
    bool thumb32_QSUB8(Reg n, Reg d, Reg m);
    bool thumb32_QSUB16(Reg n, Reg d, Reg m);
    bool thumb32_UQADD8(Reg n, Reg d, Reg m);
    bool thumb32_UQADD16(Reg n, Reg d, Reg m);
    bool thumb32_UQASX(Reg n, Reg d, Reg m);
    bool thumb32_UQSAX(Reg n, Reg d, Reg m);
    bool thumb32_UQSUB8(Reg n, Reg d, Reg m);
    bool thumb32_UQSUB16(Reg n, Reg d, Reg m);

    bool thumb32_SHADD8(Reg n, Reg d, Reg m);
    bool thumb32_SHADD16(Reg n, Reg d, Reg m);
    bool thumb32_SHASX(Reg n, Reg d, Reg m);
    bool thumb32_SHSAX(Reg n, Reg d, Reg m);
    bool thumb32_SHSUB8(Reg n, Reg d, Reg m);
    bool thumb32_SHSUB16(Reg n, Reg d, Reg m);
    bool thumb32_UHADD8(Reg n, Reg d, Reg m);
    bool thumb32_UHADD16(Reg n, Reg d, Reg m);
    bool thumb32_UHASX(Reg n, Reg d, Reg m);
    bool thumb32_UHSAX(Reg n, Reg d, Reg m);
    bool thumb32_UHSUB8(Reg n, Reg d, Reg m);
    bool thumb32_UHSUB16(Reg n, Reg d, Reg m);
};

} // namespace Dynarmic::A32
