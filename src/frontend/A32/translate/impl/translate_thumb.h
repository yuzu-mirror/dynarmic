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

    A32::IREmitter ir;
    ConditionalState cond_state = ConditionalState::None;
    TranslationOptions options;

    bool ConditionPassed(bool is_thumb_16);

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

    // thumb32 data processing (modified immediate) instructions
    bool thumb32_TST_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8);
    bool thumb32_AND_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_BIC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_MOV_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8);
    bool thumb32_ORR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8);

    // thumb32 miscellaneous control instructions
    bool thumb32_UDF();

    // thumb32 branch instructions
    bool thumb32_BL_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);
    bool thumb32_BLX_imm(Imm<1> S, Imm<10> hi, Imm<1> j1, Imm<1> j2, Imm<11> lo);

    // thumb32 data processing (register) instructions
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
