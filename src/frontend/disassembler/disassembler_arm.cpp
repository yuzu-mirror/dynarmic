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
#include "frontend/decoder/arm.h"
#include "frontend/decoder/vfp2.h"

namespace Dynarmic {
namespace Arm {

class DisassemblerVisitor {
public:
    template<typename T>
    const char* SignStr(T value) {
        return value >= 0 ? "+" : "-";
    }

    u32 rotr(u32 x, int shift) {
        shift &= 31;
        if (!shift) return x;
        return (x >> shift) | (x << (32 - shift));
    }

    u32 ArmExpandImm(int rotate, Imm8 imm8) {
        return rotr(static_cast<u32>(imm8), rotate*2);
    }

    std::string ShiftStr(ShiftType shift, Imm5 imm5) {
        switch (shift) {
            case ShiftType::LSL:
                if (imm5 == 0) return "";
                return Common::StringFromFormat(", lsl #%hhu", imm5);
            case ShiftType::LSR:
                if (imm5 == 0) return ", lsr #32";
                return Common::StringFromFormat(", lsr #%hhu", imm5);
            case ShiftType::ASR:
                if (imm5 == 0) return ", asr #32";
                return Common::StringFromFormat(", asr #%hhu", imm5);
            case ShiftType::ROR:
                if (imm5 == 0) return ", rrx";
                return Common::StringFromFormat(", ror #%hhu", imm5);
        }
        assert(false);
        return "<internal error>";
    }

    std::string RsrStr(Reg s, ShiftType shift, Reg m) {
        switch (shift){
            case ShiftType::LSL:
                return Common::StringFromFormat("%s, lsl %s", RegToString(m), RegToString(s));
            case ShiftType::LSR:
                return Common::StringFromFormat("%s, lsr %s", RegToString(m), RegToString(s));
            case ShiftType::ASR:
                return Common::StringFromFormat("%s, asr %s", RegToString(m), RegToString(s));
            case ShiftType::ROR:
                return Common::StringFromFormat("%s, ror %s", RegToString(m), RegToString(s));
        }
        assert(false);
        return "<internal error>";
    }

    std::string RorStr(Reg m, SignExtendRotation rotate) {
        switch (rotate) {
            case SignExtendRotation::ROR_0:
                return RegToString(m);
            case SignExtendRotation::ROR_8:
                return Common::StringFromFormat("%s, ror #8", RegToString(m));
            case SignExtendRotation::ROR_16:
                return Common::StringFromFormat("%s, ror #16", RegToString(m));
            case SignExtendRotation::ROR_24:
                return Common::StringFromFormat("%s, ror #24", RegToString(m));
        }
        assert(false);
        return "<internal error>";
    }

    std::string FPRegStr(bool dp_operation, size_t base, bool bit) {
        size_t reg_num;
        if (dp_operation) {
            reg_num = base + (bit ? 16 : 0);
        } else {
            reg_num = (base << 1) + (bit ? 1 : 0);
        }
        return Common::StringFromFormat("%c%zu", dp_operation ? 'd' : 's', reg_num);
    }

    std::string FPNextRegStr(bool dp_operation, size_t base, bool bit) {
        size_t reg_num;
        if (dp_operation) {
            reg_num = base + (bit ? 16 : 0);
        } else {
            reg_num = (base << 1) + (bit ? 1 : 0);
        }
        return Common::StringFromFormat("%c%zu", dp_operation ? 'd' : 's', reg_num + 1);
    }

    // Branch instructions
    std::string arm_B(Cond cond, Imm24 imm24) {
        s32 offset = Common::SignExtend<26, s32>(imm24 << 2) + 8;
        return Common::StringFromFormat("b%s %s#%i", CondToString(cond), SignStr(offset), abs(offset));
    }
    std::string arm_BL(Cond cond, Imm24 imm24) {
        s32 offset = Common::SignExtend<26, s32>(imm24 << 2) + 8;
        return Common::StringFromFormat("bl%s %s#%i", CondToString(cond), SignStr(offset), abs(offset));
    }
    std::string arm_BLX_imm(bool H, Imm24 imm24) {
        s32 offset = Common::SignExtend<26, s32>(imm24 << 2) + 8 + (H ? 2 : 0);
        return Common::StringFromFormat("blx %s#%i", SignStr(offset), abs(offset));
    }
    std::string arm_BLX_reg(Cond cond, Reg m) {
        return Common::StringFromFormat("blx%s %s", CondToString(cond), RegToString(m));
    }
    std::string arm_BX(Cond cond, Reg m) {
        return Common::StringFromFormat("bx%s %s", CondToString(cond), RegToString(m));
    }
    std::string arm_BXJ(Cond cond, Reg m) {
        return Common::StringFromFormat("bxj%s %s", CondToString(cond), RegToString(m));
    }

    // Coprocessor instructions
    std::string arm_CDP() { return "cdp <unimplemented>"; }
    std::string arm_LDC() { return "ldc <unimplemented>"; }
    std::string arm_MCR() { return "mcr <unimplemented>"; }
    std::string arm_MCRR() { return "mcrr <unimplemented>"; }
    std::string arm_MRC() { return "mrc <unimplemented>"; }
    std::string arm_MRRC() { return "mrrc <unimplemented>"; }
    std::string arm_STC() { return "stc <unimplemented>"; }

    // Data processing instructions
    std::string arm_ADC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("adc%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_ADC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("adc%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_ADC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("adc%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_ADD_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("add%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_ADD_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("add%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_ADD_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("add%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_AND_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("and%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_AND_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("and%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_AND_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("and%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_BIC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("bic%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_BIC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("bic%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_BIC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("bic%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_CMN_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("cmn%s %s, #%i", CondToString(cond), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_CMN_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("cmn%s %s, %s%s", CondToString(cond), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_CMN_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("cmn%s %s, %s", CondToString(cond), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_CMP_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("cmp%s %s, #%i", CondToString(cond), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_CMP_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("cmp%s %s, %s%s", CondToString(cond), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_CMP_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("cmp%s %s, %s", CondToString(cond), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_EOR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("eor%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_EOR_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("eor%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_EOR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("eor%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_MOV_imm(Cond cond, bool S, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("mov%s%s %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), ArmExpandImm(rotate, imm8));
    }
    std::string arm_MOV_reg(Cond cond, bool S, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("mov%s%s %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_MOV_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("mov%s%s %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RsrStr(s, shift, m).c_str());
    }
    std::string arm_MVN_imm(Cond cond, bool S, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("mvn%s%s %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), ArmExpandImm(rotate, imm8));
    }
    std::string arm_MVN_reg(Cond cond, bool S, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("mvn%s%s %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_MVN_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("mvn%s%s %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RsrStr(s, shift, m).c_str());
    }
    std::string arm_ORR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("orr%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_ORR_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("orr%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_ORR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("orr%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_RSB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("rsb%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_RSB_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("rsb%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_RSB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("rsb%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_RSC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("rsc%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_RSC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("rsc%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_RSC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("rsc%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_SBC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("sbc%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_SBC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("sbc%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_SBC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("sbc%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_SUB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("sub%s%s %s, %s, #%i", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_SUB_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("sub%s%s %s, %s, %s%s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_SUB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("sub%s%s %s, %s, %s", CondToString(cond), S ? "s" : "", RegToString(d), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_TEQ_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("teq%s %s, #%i", CondToString(cond), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_TEQ_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("teq%s %s, %s%s", CondToString(cond), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_TEQ_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("teq%s %s, %s", CondToString(cond), RegToString(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_TST_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("tst%s %s, #%i", CondToString(cond), RegToString(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_TST_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("tst%s %s, %s%s", CondToString(cond), RegToString(n), RegToString(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_TST_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("tst%s %s, %s", CondToString(cond), RegToString(n), RsrStr(s, shift, m).c_str());
    }

    // Exception generation instructions
    std::string arm_BKPT(Cond cond, Imm12 imm12, Imm4 imm4) {
        return Common::StringFromFormat("bkpt #%hu", imm12 << 4 | imm4);
    }
    std::string arm_SVC(Cond cond, Imm24 imm24) {
        return Common::StringFromFormat("svc%s #%u", CondToString(cond), imm24);
    }
    std::string arm_UDF() {
        return Common::StringFromFormat("udf");
    }

    // Extension functions
    std::string arm_SXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("sxtab%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RorStr(m, rotate).c_str());
    }
    std::string arm_SXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("sxtab16%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RorStr(m, rotate).c_str());
    }
    std::string arm_SXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("sxtah%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RorStr(m, rotate).c_str());
    }
    std::string arm_SXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("sxtb%s %s, %s", CondToString(cond), RegToString(d), RorStr(m, rotate).c_str());
    }
    std::string arm_SXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("sxtb16%s %s, %s", CondToString(cond), RegToString(d), RorStr(m, rotate).c_str());
    }
    std::string arm_SXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("sxth%s %s, %s", CondToString(cond), RegToString(d), RorStr(m, rotate).c_str());
    }
    std::string arm_UXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("uxtab%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RorStr(m, rotate).c_str());
    }
    std::string arm_UXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("uxtab16%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RorStr(m, rotate).c_str());
    }
    std::string arm_UXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("uxtah%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RorStr(m, rotate).c_str());
    }
    std::string arm_UXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("uxtb%s %s, %s", CondToString(cond), RegToString(d), RorStr(m, rotate).c_str());
    }
    std::string arm_UXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("uxtb16%s %s, %s", CondToString(cond), RegToString(d), RorStr(m, rotate).c_str());
    }
    std::string arm_UXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
        return Common::StringFromFormat("uxth%s %s, %s", CondToString(cond), RegToString(d), RorStr(m, rotate).c_str());
    }

    // Hint instructions
    std::string arm_PLD() { return "pld <unimplemented>"; }
    std::string arm_SEV() { return "sev <unimplemented>"; }
    std::string arm_WFE() { return "wfe <unimplemented>"; }
    std::string arm_WFI() { return "wfi <unimplemented>"; }
    std::string arm_YIELD() { return "yield <unimplemented>"; }

    // Load/Store instructions
    std::string arm_LDR_lit(Cond cond, bool U, Reg t, Imm12 imm12) {
        bool P = true, W = false;
        return arm_LDR_imm(cond, P, U, W, Reg::PC, t, imm12);
    }
    std::string arm_LDR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm12 imm12) {
        if (P) {
            return Common::StringFromFormat("ldr%s %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm12, W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldr%s %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm12, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm5 imm5, ShiftType shift, Reg m) {
        if (P) {
            return Common::StringFromFormat("ldr%s %s, [%s, %c%s%s]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), ShiftStr(shift, imm5).c_str(), W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldr%s %s, [%s], %c%s%s%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), ShiftStr(shift, imm5).c_str(), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRB_lit(Cond cond, bool U, Reg t, Imm12 imm12) {
        bool P = true, W = false;
        return arm_LDRB_imm(cond, P, U, W, Reg::PC, t, imm12);
    }
    std::string arm_LDRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm12 imm12) {
        if (P) {
            return Common::StringFromFormat("ldrb%s %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm12, W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrb%s %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm12, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm5 imm5, ShiftType shift, Reg m) {
        if (P) {
            return Common::StringFromFormat("ldrb%s %s, [%s, %c%s%s]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), ShiftStr(shift, imm5).c_str(), W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrb%s %s, [%s], %c%s%s%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), ShiftStr(shift, imm5).c_str(), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRBT() { return "ice"; }
    std::string arm_LDRD_lit(Cond cond, bool U, Reg t, Imm4 imm8a, Imm4 imm8b) {
        bool P = true, W = false;
        return arm_LDRD_imm(cond, P, U, W, Reg::PC, t, imm8a, imm8b);
    }
    std::string arm_LDRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
        u32 imm32 = (imm8a << 4) | imm8b;
        if (P) {
            return Common::StringFromFormat("ldrd%s %s, %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(t+1), RegToString(n), U ? '+' : '-', imm32, W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrd%s %s, %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(t+1), RegToString(n), U ? '+' : '-', imm32, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        if (P) {
            return Common::StringFromFormat("ldrd%s %s, %s, [%s, %c%s]%s", CondToString(cond), RegToString(t), RegToString(t+1), RegToString(n), U ? '+' : '-', RegToString(m), W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrd%s %s, %s, [%s], %c%s%s", CondToString(cond), RegToString(t), RegToString(t+1), RegToString(n), U ? '+' : '-', RegToString(m), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRH_lit(Cond cond, bool P, bool U, bool W, Reg t, Imm4 imm8a, Imm4 imm8b) {
        return arm_LDRH_imm(cond, P, U, W, Reg::PC, t, imm8a, imm8b);
    }
    std::string arm_LDRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
        u32 imm32 = (imm8a << 4) | imm8b;
        if (P) {
            return Common::StringFromFormat("ldrh%s %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm32, W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrh%s %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm32, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        if (P) {
            return Common::StringFromFormat("ldrd%s %s, [%s, %c%s]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrd%s %s, [%s], %c%s%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRHT() { return "ice"; }
    std::string arm_LDRSB_lit(Cond cond, bool U, Reg t, Imm4 imm8a, Imm4 imm8b) {
        bool P = true, W = false;
        return arm_LDRSB_imm(cond, P, U, W, Reg::PC, t, imm8a, imm8b);
    }
    std::string arm_LDRSB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
        u32 imm32 = (imm8a << 4) | imm8b;
        if (P) {
            return Common::StringFromFormat("ldrsb%s %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm32, W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrsb%s %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm32, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRSB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        if (P) {
            return Common::StringFromFormat("ldrsb%s %s, [%s, %c%s]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrsb%s %s, [%s], %c%s%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRSBT() { return "ice"; }
    std::string arm_LDRSH_lit(Cond cond, bool U, Reg t, Imm4 imm8a, Imm4 imm8b) {
        bool P = true, W = false;
        return arm_LDRSH_imm(cond, P, U, W, Reg::PC, t, imm8a, imm8b);
    }
    std::string arm_LDRSH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
        u32 imm32 = (imm8a << 4) | imm8b;
        if (P) {
            return Common::StringFromFormat("ldrsh%s %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm32, W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrsh%s %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm32, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRSH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        if (P) {
            return Common::StringFromFormat("ldrsh%s %s, [%s, %c%s]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), W ? "!" : "");
        } else {
            return Common::StringFromFormat("ldrsh%s %s, [%s], %c%s%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_LDRSHT() { return "ice"; }
    std::string arm_LDRT() { return "ice"; }
    std::string arm_STR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm12 imm12) {
        if (P) {
            return Common::StringFromFormat("str%s %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm12, W ? "!" : "");
        } else {
            return Common::StringFromFormat("str%s %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm12, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm5 imm5, ShiftType shift, Reg m) {
        if (P) {
            return Common::StringFromFormat("str%s %s, [%s, %c%s%s]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), ShiftStr(shift, imm5).c_str(), W ? "!" : "");
        } else {
            return Common::StringFromFormat("str%s %s, [%s], %c%s%s%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), ShiftStr(shift, imm5).c_str(), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm12 imm12) {
        if (P) {
            return Common::StringFromFormat("strb%s %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm12, W ? "!" : "");
        } else {
            return Common::StringFromFormat("strb%s %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm12, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm5 imm5, ShiftType shift, Reg m) {
        if (P) {
            return Common::StringFromFormat("strb%s %s, [%s, %c%s%s]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), ShiftStr(shift, imm5).c_str(), W ? "!" : "");
        } else {
            return Common::StringFromFormat("strb%s %s, [%s], %c%s%s%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), ShiftStr(shift, imm5).c_str(), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRBT() { return "ice"; }
    std::string arm_STRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
        u32 imm32 = (imm8a << 4) | imm8b;
        if (P) {
            return Common::StringFromFormat("strd%s %s, %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(t+1), RegToString(n), U ? '+' : '-', imm32, W ? "!" : "");
        } else {
            return Common::StringFromFormat("strd%s %s, %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(t+1), RegToString(n), U ? '+' : '-', imm32, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        if (P) {
            return Common::StringFromFormat("strd%s %s, %s, [%s, %c%s]%s", CondToString(cond), RegToString(t), RegToString(t+1), RegToString(n), U ? '+' : '-', RegToString(m), W ? "!" : "");
        } else {
            return Common::StringFromFormat("strd%s %s, %s, [%s], %c%s%s", CondToString(cond), RegToString(t), RegToString(t+1), RegToString(n), U ? '+' : '-', RegToString(m), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm4 imm8a, Imm4 imm8b) {
        u32 imm32 = (imm8a << 4) | imm8b;
        if (P) {
            return Common::StringFromFormat("strh%s %s, [%s, #%c%u]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm32, W ? "!" : "");
        } else {
            return Common::StringFromFormat("strh%s %s, [%s], #%c%u%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', imm32, W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
        if (P) {
            return Common::StringFromFormat("strd%s %s, [%s, %c%s]%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), W ? "!" : "");
        } else {
            return Common::StringFromFormat("strd%s %s, [%s], %c%s%s", CondToString(cond), RegToString(t), RegToString(n), U ? '+' : '-', RegToString(m), W ? " (err: W == 1!!!)" : "");
        }
    }
    std::string arm_STRHT() { return "ice"; }
    std::string arm_STRT() { return "ice"; }

    // Load/Store multiple instructions
    std::string arm_LDM(Cond cond, bool W, Reg n, RegList list) {
        return Common::StringFromFormat("ldm%s %s%s, {%s}", CondToString(cond), RegToString(n), W ? "!" : "", RegListToString(list).c_str());
    }
    std::string arm_LDMDA(Cond cond, bool W, Reg n, RegList list) {
        return Common::StringFromFormat("ldmda%s %s%s, {%s}", CondToString(cond), RegToString(n), W ? "!" : "", RegListToString(list).c_str());
    }
    std::string arm_LDMDB(Cond cond, bool W, Reg n, RegList list) {
        return Common::StringFromFormat("ldmdb%s %s%s, {%s}", CondToString(cond), RegToString(n), W ? "!" : "", RegListToString(list).c_str());
    }
    std::string arm_LDMIB(Cond cond, bool W, Reg n, RegList list) {
        return Common::StringFromFormat("ldmib%s %s%s, {%s}", CondToString(cond), RegToString(n), W ? "!" : "", RegListToString(list).c_str());
    }
    std::string arm_LDM_usr() { return "ice"; }
    std::string arm_LDM_eret() { return "ice"; }
    std::string arm_STM(Cond cond, bool W, Reg n, RegList list) {
        return Common::StringFromFormat("stm%s %s%s, {%s}", CondToString(cond), RegToString(n), W ? "!" : "", RegListToString(list).c_str());
    }
    std::string arm_STMDA(Cond cond, bool W, Reg n, RegList list) {
        return Common::StringFromFormat("stmda%s %s%s, {%s}", CondToString(cond), RegToString(n), W ? "!" : "", RegListToString(list).c_str());
    }
    std::string arm_STMDB(Cond cond, bool W, Reg n, RegList list) {
        return Common::StringFromFormat("stmdb%s %s%s, {%s}", CondToString(cond), RegToString(n), W ? "!" : "", RegListToString(list).c_str());
    }
    std::string arm_STMIB(Cond cond, bool W, Reg n, RegList list) {
        return Common::StringFromFormat("stmib%s %s%s, {%s}", CondToString(cond), RegToString(n), W ? "!" : "", RegListToString(list).c_str());
    }
    std::string arm_STM_usr() { return "ice"; }

    // Miscellaneous instructions
    std::string arm_CLZ(Cond cond, Reg d, Reg m) {
        return Common::StringFromFormat("clz%s %s, %s", CondToString(cond), RegToString(d), RegToString(m));
    }
    std::string arm_NOP() {
        return "nop";
    }
    std::string arm_SEL(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("sel%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }

    // Unsigned sum of absolute difference functions
    std::string arm_USAD8(Cond cond, Reg d, Reg m, Reg n) {
        return Common::StringFromFormat("usad8%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }
    std::string arm_USADA8(Cond cond, Reg d, Reg a, Reg m, Reg n) {
        return Common::StringFromFormat("usad8a%s %s, %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m), RegToString(a));
    }

    // Packing instructions
    std::string arm_PKHBT(Cond cond, Reg n, Reg d, Imm5 imm5, Reg m) {
        return Common::StringFromFormat("pkhbt%s %s, %s, %s%s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m), ShiftStr(ShiftType::LSL, imm5).c_str());
    }
    std::string arm_PKHTB(Cond cond, Reg n, Reg d, Imm5 imm5, Reg m) {
        return Common::StringFromFormat("pkhtb%s %s, %s, %s%s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m), ShiftStr(ShiftType::ASR, imm5).c_str());
    }

    // Reversal instructions
    std::string arm_REV(Cond cond, Reg d, Reg m) {
        return Common::StringFromFormat("rev%s %s, %s", CondToString(cond), RegToString(d), RegToString(m));
    }
    std::string arm_REV16(Cond cond, Reg d, Reg m) {
        return Common::StringFromFormat("rev16%s %s, %s", CondToString(cond), RegToString(d), RegToString(m));
    }
    std::string arm_REVSH(Cond cond, Reg d, Reg m) {
        return Common::StringFromFormat("revsh%s %s, %s", CondToString(cond), RegToString(d), RegToString(m));
    }

    // Saturation instructions
    std::string arm_SSAT(Cond cond, Imm5 sat_imm, Reg d, Imm5 imm5, bool sh, Reg n) {
        return Common::StringFromFormat("ssat%s %s, #%u, %s%s", CondToString(cond), RegToString(d), sat_imm + 1, RegToString(n), ShiftStr(ShiftType(sh << 1), imm5).c_str());
    }
    std::string arm_SSAT16(Cond cond, Imm4 sat_imm, Reg d, Reg n) {
        return Common::StringFromFormat("ssat16%s %s, #%u, %s", CondToString(cond), RegToString(d), sat_imm + 1, RegToString(n));
    }
    std::string arm_USAT(Cond cond, Imm5 sat_imm, Reg d, Imm5 imm5, bool sh, Reg n) {
        return Common::StringFromFormat("usat%s %s, #%u, %s%s", CondToString(cond), RegToString(d), sat_imm, RegToString(n), ShiftStr(ShiftType(sh << 1), imm5).c_str());
    }
    std::string arm_USAT16(Cond cond, Imm4 sat_imm, Reg d, Reg n) {
        return Common::StringFromFormat("usat16%s %s, #%u, %s", CondToString(cond), RegToString(d), sat_imm, RegToString(n));
    }

    // Multiply (Normal) instructions
    std::string arm_MLA(Cond cond, bool S, Reg d, Reg a, Reg m, Reg n) {
        return Common::StringFromFormat("mla%s%s %s, %s, %s, %s", S ? "s" : "", CondToString(cond), RegToString(d), RegToString(n), RegToString(m), RegToString(a));
    }
    std::string arm_MUL(Cond cond, bool S, Reg d, Reg m, Reg n) {
        return Common::StringFromFormat("mul%s%s %s, %s, %s", S ? "s" : "", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }

    // Multiply (Long) instructions
    std::string arm_SMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
        return Common::StringFromFormat("smlal%s%s %s, %s, %s, %s", S ? "s" : "", CondToString(cond), RegToString(dLo), RegToString(dHi), RegToString(n), RegToString(m));
    }
    std::string arm_SMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
        return Common::StringFromFormat("smull%s%s %s, %s, %s, %s", S ? "s" : "", CondToString(cond), RegToString(dLo), RegToString(dHi), RegToString(n), RegToString(m));
    }
    std::string arm_UMAAL(Cond cond, Reg dHi, Reg dLo, Reg m, Reg n) {
        return Common::StringFromFormat("umaal%s %s, %s, %s, %s", CondToString(cond), RegToString(dLo), RegToString(dHi), RegToString(n), RegToString(m));
    }
    std::string arm_UMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
        return Common::StringFromFormat("umlal%s%s %s, %s, %s, %s", S ? "s" : "", CondToString(cond), RegToString(dLo), RegToString(dHi), RegToString(n), RegToString(m));
    }
    std::string arm_UMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
        return Common::StringFromFormat("umull%s%s %s, %s, %s, %s", S ? "s" : "", CondToString(cond), RegToString(dLo), RegToString(dHi), RegToString(n), RegToString(m));
    }

    // Multiply (Halfword) instructions
    std::string arm_SMLALxy(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, bool N, Reg n) {
        return Common::StringFromFormat("smlal%c%c%s %s, %s, %s, %s", N ? 't' : 'b', M ? 't' : 'b', CondToString(cond), RegToString(dLo), RegToString(dHi), RegToString(n), RegToString(m));
    }
    std::string arm_SMLAxy(Cond cond, Reg d, Reg a, Reg m, bool M, bool N, Reg n) {
        return Common::StringFromFormat("smla%c%c%s %s, %s, %s, %s", N ? 't' : 'b', M ? 't' : 'b', CondToString(cond), RegToString(d), RegToString(n), RegToString(m), RegToString(a));
    }
    std::string arm_SMULxy(Cond cond, Reg d, Reg m, bool M, bool N, Reg n) {
        return Common::StringFromFormat("smul%c%c%s %s, %s, %s", N ? 't' : 'b', M ? 't' : 'b', CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }

    // Multiply (word by halfword) instructions
    std::string arm_SMLAWy(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
        return Common::StringFromFormat("smlaw%c%s %s, %s, %s, %s", M ? 't' : 'b', CondToString(cond), RegToString(d), RegToString(n), RegToString(m), RegToString(a));
    }
    std::string arm_SMULWy(Cond cond, Reg d, Reg m, bool M, Reg n) {
        return Common::StringFromFormat("smulw%c%s %s, %s, %s", M ? 't' : 'b', CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }

    // Multiply (Most significant word) instructions
    std::string arm_SMMLA(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) {
        return Common::StringFromFormat("smmla%s%s %s, %s, %s, %s", R ? "r" : "", CondToString(cond), RegToString(d), RegToString(n), RegToString(m), RegToString(a));
    }
    std::string arm_SMMLS(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) {
        return Common::StringFromFormat("smmls%s%s %s, %s, %s, %s", R ? "r" : "", CondToString(cond), RegToString(d), RegToString(n), RegToString(m), RegToString(a));
    }
    std::string arm_SMMUL(Cond cond, Reg d, Reg m, bool R, Reg n) {
        return Common::StringFromFormat("smmul%s%s %s, %s, %s", R ? "r" : "", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }

    // Multiply (Dual) instructions
    std::string arm_SMLAD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
        return Common::StringFromFormat("smlad%s%s %s, %s, %s, %s", M ? "x" : "", CondToString(cond), RegToString(d), RegToString(n), RegToString(m), RegToString(a));
    }
    std::string arm_SMLALD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) {
        return Common::StringFromFormat("smlald%s%s %s, %s, %s, %s", M ? "x" : "", CondToString(cond), RegToString(dLo), RegToString(dHi), RegToString(n), RegToString(m));
    }
    std::string arm_SMLSD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
        return Common::StringFromFormat("smlsd%s%s %s, %s, %s, %s", M ? "x" : "", CondToString(cond), RegToString(d), RegToString(n), RegToString(m), RegToString(a));
    }
    std::string arm_SMLSLD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) {
        return Common::StringFromFormat("smlsld%s%s %s, %s, %s, %s", M ? "x" : "", CondToString(cond), RegToString(dLo), RegToString(dHi), RegToString(n), RegToString(m));
    }
    std::string arm_SMUAD(Cond cond, Reg d, Reg m, bool M, Reg n) {
        return Common::StringFromFormat("smuad%s%s %s, %s, %s", M ? "x" : "", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }
    std::string arm_SMUSD(Cond cond, Reg d, Reg m, bool M, Reg n) {
        return Common::StringFromFormat("smusd%s%s %s, %s, %s", M ? "x" : "", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }

    // Parallel Add/Subtract (Modulo arithmetic) instructions
    std::string arm_SADD8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SADD16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SASX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SSAX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SSUB8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SSUB16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UADD8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UADD16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UASX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_USAX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_USUB8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_USUB16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }

    // Parallel Add/Subtract (Saturating) instructions
    std::string arm_QADD8(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("qadd8%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }
    std::string arm_QADD16(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("qadd16%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }
    std::string arm_QASX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QSAX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QSUB8(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("qsub8%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }
    std::string arm_QSUB16(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("qsub16%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }
    std::string arm_UQADD8(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("uqadd8%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }
    std::string arm_UQADD16(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("uqadd16%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }
    std::string arm_UQASX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UQSAX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UQSUB8(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("uqsub8%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }
    std::string arm_UQSUB16(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("uqsub16%s %s, %s, %s", CondToString(cond), RegToString(d), RegToString(n), RegToString(m));
    }

    // Parallel Add/Subtract (Halving) instructions
    std::string arm_SHADD8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SHADD16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SHASX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SHSAX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SHSUB8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SHSUB16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UHADD8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UHADD16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UHASX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UHSAX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UHSUB8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UHSUB16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }

    // Saturated Add/Subtract instructions
    std::string arm_QADD(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QSUB(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QDADD(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QDSUB(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }

    // Synchronization Primitive instructions
    std::string arm_CLREX() {
        return "clrex";
    }
    std::string arm_LDREX(Cond cond, Reg n, Reg d) {
        return Common::StringFromFormat("ldrex%s %s, [%s]", CondToString(cond), RegToString(d), RegToString(n));
    }
    std::string arm_LDREXB(Cond cond, Reg n, Reg d) {
        return Common::StringFromFormat("ldrexb%s %s, [%s]", CondToString(cond), RegToString(d), RegToString(n));
    }
    std::string arm_LDREXD(Cond cond, Reg n, Reg d) {
        return Common::StringFromFormat("ldrexd%s %s, %s, [%s]", CondToString(cond), RegToString(d), RegToString(d+1), RegToString(n));
    }
    std::string arm_LDREXH(Cond cond, Reg n, Reg d) {
        return Common::StringFromFormat("ldrexh%s %s, [%s]", CondToString(cond), RegToString(d), RegToString(n));
    }
    std::string arm_STREX(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("strex%s %s, %s, [%s]", CondToString(cond), RegToString(d), RegToString(m), RegToString(n));
    }
    std::string arm_STREXB(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("strexb%s %s, %s, [%s]", CondToString(cond), RegToString(d), RegToString(m), RegToString(n));
    }
    std::string arm_STREXD(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("strexd%s %s, %s, %s, [%s]", CondToString(cond), RegToString(d), RegToString(m), RegToString(m+1), RegToString(n));
    }
    std::string arm_STREXH(Cond cond, Reg n, Reg d, Reg m) {
        return Common::StringFromFormat("strexh%s %s, %s, [%s]", CondToString(cond), RegToString(d), RegToString(m), RegToString(n));
    }
    std::string arm_SWP(Cond cond, Reg n, Reg t, Reg t2) {
        return Common::StringFromFormat("swp%s %s, %s, [%s]", CondToString(cond), RegToString(t), RegToString(t2), RegToString(n));
    }
    std::string arm_SWPB(Cond cond, Reg n, Reg t, Reg t2) {
        return Common::StringFromFormat("swpb%s %s, %s, [%s]", CondToString(cond), RegToString(t), RegToString(t2), RegToString(n));
    }

    // Status register access instructions
    std::string arm_CPS() { return "ice"; }
    std::string arm_MRS(Cond cond, Reg d) {
        return Common::StringFromFormat("mrs%s %s, apsr", CondToString(cond), RegToString(d));
    }
    std::string arm_MSR_imm(Cond cond, int mask, int rotate, Imm8 imm8) {
        bool write_nzcvq = Common::Bit<1>(mask);
        bool write_g = Common::Bit<0>(mask);
        return Common::StringFromFormat("msr%s apsr_%s%s, #%u", CondToString(cond), write_nzcvq ? "nzcvq" : "", write_g ? "g" : "", ArmExpandImm(rotate, imm8));
    }
    std::string arm_MSR_reg(Cond cond, int mask, Reg n) {
        bool write_nzcvq = Common::Bit<1>(mask);
        bool write_g = Common::Bit<0>(mask);
        return Common::StringFromFormat("msr%s apsr_%s%s, %s", CondToString(cond), write_nzcvq ? "nzcvq" : "", write_g ? "g" : "", RegToString(n));
    }
    std::string arm_RFE() { return "ice"; }
    std::string arm_SETEND(bool E) {
        return E ? "setend be" : "setend le";
    }
    std::string arm_SRS() { return "ice"; }

    // Floating point arithmetic instructions
    std::string vfp2_VADD(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return Common::StringFromFormat("vadd%s.%s %s, %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vn, N).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VSUB(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return Common::StringFromFormat("vsub%s.%s %s, %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vn, N).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return Common::StringFromFormat("vmul%s.%s %s, %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vn, N).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return Common::StringFromFormat("vmla%s.%s %s, %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vn, N).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return Common::StringFromFormat("vmls%s.%s %s, %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vn, N).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VNMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return Common::StringFromFormat("vnmul%s.%s %s, %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vn, N).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VNMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return Common::StringFromFormat("vnmla%s.%s %s, %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vn, N).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VNMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return Common::StringFromFormat("vnmls%s.%s %s, %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vn, N).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VDIV(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
        return Common::StringFromFormat("vdiv%s.%s %s, %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vn, N).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VMOV_u32_f64(Cond cond, size_t Vd, Reg t, bool D){
        return Common::StringFromFormat("vmov%s.32 %s, %s", CondToString(cond), FPRegStr(true, Vd, D).c_str(), RegToString(t));
    }

    std::string vfp2_VMOV_f64_u32(Cond cond, size_t Vn, Reg t, bool N){
        return Common::StringFromFormat("vmov%s.32 %s, %s", CondToString(cond), RegToString(t), FPRegStr(true, Vn, N).c_str());
    }

    std::string vfp2_VMOV_u32_f32(Cond cond, size_t Vn, Reg t, bool N){
        return Common::StringFromFormat("vmov%s.32 %s, %s", CondToString(cond), FPRegStr(false, Vn, N).c_str(), RegToString(t));
    }

    std::string vfp2_VMOV_f32_u32(Cond cond, size_t Vn, Reg t, bool N){
        return Common::StringFromFormat("vmov%s.32 %s, %s", CondToString(cond), RegToString(t), FPRegStr(false, Vn, N).c_str());
    }

    std::string vfp2_VMOV_2u32_2f32(Cond cond, Reg t2, Reg t, bool M, size_t Vm){
        return Common::StringFromFormat("vmov%s %s, %s, %s, %s", CondToString(cond), FPRegStr(false, Vm, M).c_str(), FPNextRegStr(false, Vm, M).c_str(), RegToString(t), RegToString(t2));
    }

    std::string vfp2_VMOV_2f32_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm){
        return Common::StringFromFormat("vmov%s %s, %s, %s, %s", CondToString(cond), RegToString(t), RegToString(t2), FPRegStr(false, Vm, M).c_str(), FPNextRegStr(false, Vm, M).c_str());
    }

    std::string vfp2_VMOV_2u32_f64(Cond cond, Reg t2, Reg t, bool M, size_t Vm){
        return Common::StringFromFormat("vmov%s %s, %s, %s", CondToString(cond), FPRegStr(true, Vm, M).c_str(), RegToString(t), RegToString(t2));
    }

    std::string vfp2_VMOV_f64_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm){
        return Common::StringFromFormat("vmov%s %s, %s, %s", CondToString(cond), RegToString(t), RegToString(t2), FPRegStr(true, Vm, M).c_str());
    }

    std::string vfp2_VMOV_reg(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm){
        return Common::StringFromFormat("vmov%s.%s %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VABS(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return Common::StringFromFormat("vadd%s.%s %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VNEG(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return Common::StringFromFormat("vneg%s.%s %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string vfp2_VSQRT(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
        return Common::StringFromFormat("vsqrt%s.%s %s, %s", CondToString(cond), sz ? "f64" : "f32", FPRegStr(sz, Vd, D).c_str(), FPRegStr(sz, Vm, M).c_str());
    }

    std::string  vfp2_VPOP(Cond cond, bool D, size_t Vd, bool sz, Imm8 imm8) {
        return Common::StringFromFormat("vpop%s %s(+%u)", CondToString(cond), FPRegStr(sz, Vd, D).c_str(), imm8 >> (sz ? 1 : 0));
    }

    std::string  vfp2_VPUSH(Cond cond, bool D, size_t Vd, bool sz, Imm8 imm8) {
        return Common::StringFromFormat("vpush%s %s(+%u)", CondToString(cond), FPRegStr(sz, Vd, D).c_str(), imm8 >> (sz ? 1 : 0));
    }

    std::string vfp2_VLDR(Cond cond, bool U, bool D, Reg n, size_t Vd, bool sz, Imm8 imm8) {
        u32 imm32 = imm8 << 2;
        return Common::StringFromFormat("vldr%s %s, [%s, #%c%u]", CondToString(cond), FPRegStr(sz, Vd, D).c_str(), RegToString(n), U ? '+' : '-', imm32);
    }

    std::string vfp2_VSTR(Cond cond, bool U, bool D, Reg n, size_t Vd, bool sz, Imm8 imm8) {
        u32 imm32 = imm8 << 2;
        return Common::StringFromFormat("vstr%s %s, [%s, #%c%u]", CondToString(cond), FPRegStr(sz, Vd, D).c_str(), RegToString(n), U ? '+' : '-', imm32);
    }
};

std::string DisassembleArm(u32 instruction) {
    DisassemblerVisitor visitor;
    if (auto vfp_decoder = DecodeVFP2<DisassemblerVisitor>(instruction)) {
        return vfp_decoder->call(visitor, instruction);
    } else if (auto decoder = DecodeArm<DisassemblerVisitor>(instruction)) {
        return decoder->call(visitor, instruction);
    } else {
        return Common::StringFromFormat("UNKNOWN: %x", instruction);
    }
}

} // namespace Arm
} // namespace Dynarmic
