/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <cstdlib>
#include <string>

#include "common/bit_util.h"
#include "common/string_util.h"
#include "frontend/arm_types.h"
#include "frontend/decoder/thumb16.h"

namespace Dynarmic {
namespace Arm {

class DisassemblerVisitor {
public:
    std::string thumb16_LSL_imm(Imm5 imm5, Reg m, Reg d) {
        return Common::StringFromFormat("lsls %s, %s, #%u", RegToString(d), RegToString(m), imm5);
    }

    std::string thumb16_LSR_imm(Imm5 imm5, Reg m, Reg d) {
        return Common::StringFromFormat("lsrs %s, %s, #%u", RegToString(d), RegToString(m), imm5);
    }

    std::string thumb16_ASR_imm(Imm5 imm5, Reg m, Reg d) {
        return Common::StringFromFormat("asrs %s, %s, #%u", RegToString(d), RegToString(m), imm5);
    }

    std::string thumb16_ADD_reg_t1(Reg m, Reg n, Reg d) {
        return Common::StringFromFormat("adds %s, %s, %s", RegToString(d), RegToString(n), RegToString(m));
    }

    std::string thumb16_SUB_reg(Reg m, Reg n, Reg d) {
        return Common::StringFromFormat("subs %s, %s, %s", RegToString(d), RegToString(n), RegToString(m));
    }

    std::string thumb16_ADD_imm_t1(Imm3 imm3, Reg n, Reg d) {
        return Common::StringFromFormat("adds %s, %s, #%u", RegToString(d), RegToString(n), imm3);
    }

    std::string thumb16_SUB_imm_t1(Imm3 imm3, Reg n, Reg d) {
        return Common::StringFromFormat("subs %s, %s, #%u", RegToString(d), RegToString(n), imm3);
    }

    std::string thumb16_MOV_imm(Reg d, Imm8 imm8) {
        return Common::StringFromFormat("movs %s, #%u", RegToString(d), imm8);
    }

    std::string thumb16_CMP_imm(Reg n, Imm8 imm8) {
        return Common::StringFromFormat("cmp %s, #%u", RegToString(n), imm8);
    }

    std::string thumb16_ADD_imm_t2(Reg d_n, Imm8 imm8) {
        return Common::StringFromFormat("adds %s, #%u", RegToString(d_n), imm8);
    }

    std::string thumb16_SUB_imm_t2(Reg d_n, Imm8 imm8) {
        return Common::StringFromFormat("subs %s, #%u", RegToString(d_n), imm8);
    }

    std::string thumb16_AND_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("ands %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_EOR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("eors %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_LSL_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("lsls %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_LSR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("lsrs %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_ASR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("asrs %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_ADC_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("adcs %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_SBC_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("sbcs %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_ROR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("rors %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_TST_reg(Reg m, Reg n) {
        return Common::StringFromFormat("tst %s, %s", RegToString(n), RegToString(m));
    }

    std::string thumb16_RSB_imm(Reg n, Reg d) {
        // Pre-UAL syntax: NEGS <Rd>, <Rn>
        return Common::StringFromFormat("rsbs %s, %s, #0", RegToString(d), RegToString(n));
    }

    std::string thumb16_CMP_reg_t1(Reg m, Reg n) {
        return Common::StringFromFormat("cmp %s, %s", RegToString(n), RegToString(m));
    }

    std::string thumb16_CMN_reg(Reg m, Reg n) {
        return Common::StringFromFormat("cmn %s, %s", RegToString(n), RegToString(m));
    }

    std::string thumb16_ORR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("orrs %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_BIC_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("bics %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_MVN_reg(Reg m, Reg d) {
        return Common::StringFromFormat("mvns %s, %s", RegToString(d), RegToString(m));
    }

    std::string thumb16_ADD_reg_t2(bool d_n_hi, Reg m, Reg d_n_lo) {
        Reg d_n = d_n_hi ? (d_n_lo + 8) : d_n_lo;
        return Common::StringFromFormat("add %s, %s", RegToString(d_n), RegToString(m));
    }

    std::string thumb16_CMP_reg_t2(bool n_hi, Reg m, Reg n_lo) {
        Reg n = n_hi ? (n_lo + 8) : n_lo;
        return Common::StringFromFormat("cmp %s, %s", RegToString(n), RegToString(m));
    }

    std::string thumb16_MOV_reg(bool d_hi, Reg m, Reg d_lo) {
        Reg d = d_hi ? (d_lo + 8) : d_lo;
        return Common::StringFromFormat("mov %s, %s", RegToString(d), RegToString(m));
    }

    std::string thumb16_LDR_literal(Reg t, Imm8 imm8) {
        u32 imm32 = imm8 << 2;
        return Common::StringFromFormat("ldr %s, [pc, #%u]", RegToString(t), imm32);
    }

    std::string thumb16_STR_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("str %s, [%s, %s]", RegToString(t), RegToString(n), RegToString(m));
    }

    std::string thumb16_STRH_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("strh %s, [%s, %s]", RegToString(t), RegToString(n), RegToString(m));
    }

    std::string thumb16_STRB_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("strb %s, [%s, %s]", RegToString(t), RegToString(n), RegToString(m));
    }

    std::string thumb16_LDRSB_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("ldrsb %s, [%s, %s]", RegToString(t), RegToString(n), RegToString(m));
    }

    std::string thumb16_LDR_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("ldr %s, [%s, %s]", RegToString(t), RegToString(n), RegToString(m));
    }

    std::string thumb16_LDRH_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("ldrh %s, [%s, %s]", RegToString(t), RegToString(n), RegToString(m));
    }

    std::string thumb16_LDRB_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("ldrb %s, [%s, %s]", RegToString(t), RegToString(n), RegToString(m));
    }

    std::string thumb16_LDRSH_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("ldrsh %s, [%s, %s]", RegToString(t), RegToString(n), RegToString(m));
    }

    std::string thumb16_STR_imm_t1(Imm5 imm5, Reg n, Reg t) {
        u32 imm32 = imm5 << 2;
        return Common::StringFromFormat("str %s, [%s, #%u]", RegToString(t), RegToString(n), imm32);
    }

    std::string thumb16_LDR_imm_t1(Imm5 imm5, Reg n, Reg t) {
        u32 imm32 = imm5 << 2;
        return Common::StringFromFormat("ldr %s, [%s, #%u]", RegToString(t), RegToString(n), imm32);
    }

    std::string thumb16_STRB_imm(Imm5 imm5, Reg n, Reg t) {
        u32 imm32 = imm5;
        return Common::StringFromFormat("strb %s, [%s, #%u]", RegToString(t), RegToString(n), imm32);
    }

    std::string thumb16_LDRB_imm(Imm5 imm5, Reg n, Reg t) {
        u32 imm32 = imm5;
        return Common::StringFromFormat("ldrb %s, [%s, #%u]", RegToString(t), RegToString(n), imm32);
    }

    std::string thumb16_STRH_imm(Imm5 imm5, Reg n, Reg t) {
        u32 imm32 = imm5 << 1;
        return Common::StringFromFormat("strh %s, [%s, #%u]", RegToString(t), RegToString(n), imm32);
    }

    std::string thumb16_LDRH_imm(Imm5 imm5, Reg n, Reg t) {
        u32 imm32 = imm5 << 1;
        return Common::StringFromFormat("ldrh %s, [%s, #%u]", RegToString(t), RegToString(n), imm32);
    }

    std::string thumb16_STR_imm_t2(Reg t, Imm5 imm5) {
        u32 imm32 = imm5 << 2;
        return Common::StringFromFormat("str %s, [sp, #%u]", RegToString(t), imm32);
    }

    std::string thumb16_LDR_imm_t2(Reg t, Imm5 imm5) {
        u32 imm32 = imm5 << 2;
        return Common::StringFromFormat("ldr %s, [sp, #%u]", RegToString(t), imm32);
    }

    std::string thumb16_ADR(Reg d, Imm8 imm8) {
        u32 imm32 = imm8 << 2;
        return Common::StringFromFormat("adr %s, +#%u", RegToString(d), imm32);
    }

    std::string thumb16_ADD_sp_t1(Reg d, Imm8 imm8) {
        u32 imm32 = imm8 << 2;
        return Common::StringFromFormat("add %s, sp, #%u", RegToString(d), imm32);
    }

    std::string thumb16_ADD_sp_t2(Imm7 imm7) {
        u32 imm32 = imm7 << 2;
        return Common::StringFromFormat("add sp, sp, #%u", imm32);
    }

    std::string thumb16_SUB_sp(Imm7 imm7) {
        u32 imm32 = imm7 << 2;
        return Common::StringFromFormat("sub sp, sp, #%u", imm32);
    }

    std::string thumb16_SXTH(Reg m, Reg d) {
        return Common::StringFromFormat("sxth %s, %s", RegToString(d), RegToString(m));
    }

    std::string thumb16_SXTB(Reg m, Reg d) {
        return Common::StringFromFormat("sxtb %s, %s", RegToString(d), RegToString(m));
    }

    std::string thumb16_UXTH(Reg m, Reg d) {
        return Common::StringFromFormat("uxth %s, %s", RegToString(d), RegToString(m));
    }

    std::string thumb16_UXTB(Reg m, Reg d) {
        return Common::StringFromFormat("uxtb %s, %s", RegToString(d), RegToString(m));
    }

    std::string thumb16_PUSH(bool M, RegList reg_list) {
        if (M) reg_list |= 1 << 14;
        return "push " + RegListToString(reg_list);
    }

    std::string thumb16_POP(bool P, RegList reg_list) {
        if (P) reg_list |= 1 << 15;
        return "pop " + RegListToString(reg_list);
    }

    std::string thumb16_SETEND(bool E) {
        return Common::StringFromFormat("setend %s", E ? "BE" : "LE");
    }

    std::string thumb16_REV(Reg m, Reg d) {
        return Common::StringFromFormat("rev %s, %s", RegToString(d), RegToString(m));
    }

    std::string thumb16_REV16(Reg m, Reg d) {
        return Common::StringFromFormat("rev16 %s, %s", RegToString(d), RegToString(m));
    }

    std::string thumb16_REVSH(Reg m, Reg d) {
        return Common::StringFromFormat("revsh %s, %s", RegToString(d), RegToString(m));
    }

    std::string thumb16_STMIA(Reg n, RegList reg_list) {
        return Common::StringFromFormat("stm %s!, %s", RegToString(n), RegListToString(reg_list).c_str());
    }

    std::string thumb16_LDMIA(Reg n, RegList reg_list) {
        bool write_back = !Dynarmic::Common::Bit(static_cast<size_t>(n), reg_list);
        return Common::StringFromFormat("ldm %s%s, %s", RegToString(n), write_back ? "!" : "", RegListToString(reg_list).c_str());
    }

    std::string thumb16_BX(Reg m) {
        return Common::StringFromFormat("bx %s", RegToString(m));
    }

    std::string thumb16_BLX_reg(Reg m) {
        return Common::StringFromFormat("blx %s", RegToString(m));
    }

    std::string thumb16_UDF() {
        return Common::StringFromFormat("udf");
    }

    std::string thumb16_SVC(Imm8 imm8) {
        return Common::StringFromFormat("svc #%u", imm8);
    }

    std::string thumb16_B_t1(Cond cond, Imm8 imm8) {
        s32 imm32 = Common::SignExtend<9, s32>(imm8 << 1) + 4;
        return Common::StringFromFormat("b%s %c#%u", CondToString(cond), Common::SignToChar(imm32), abs(imm32));
    }

    std::string thumb16_B_t2(Imm11 imm11) {
        s32 imm32 = Common::SignExtend<12, s32>(imm11 << 1) + 4;
        return Common::StringFromFormat("b %c#%u", Common::SignToChar(imm32), abs(imm32));
    }
};

std::string DisassembleThumb16(u16 instruction) {
    DisassemblerVisitor visitor;
    auto decoder = DecodeThumb16<DisassemblerVisitor>(instruction);
    return !decoder ? Common::StringFromFormat("UNKNOWN: %x", instruction) : decoder->call(visitor, instruction);
}

} // namespace Arm
} // namespace Dynarmic
