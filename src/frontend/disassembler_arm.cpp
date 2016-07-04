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

namespace Dynarmic {
namespace Arm {

class DisassemblerVisitor {
public:
    const char* CondStr(Cond cond) {
        switch (cond) {
            case Cond::EQ:
                return "eq";
            case Cond::NE:
                return "ne";
            case Cond::CS:
                return "cs";
            case Cond::CC:
                return "cc";
            case Cond::MI:
                return "mi";
            case Cond::PL:
                return "pl";
            case Cond::VS:
                return "vs";
            case Cond::VC:
                return "vc";
            case Cond::HI:
                return "hi";
            case Cond::LS:
                return "ls";
            case Cond::GE:
                return "ge";
            case Cond::LT:
                return "lt";
            case Cond::GT:
                return "gt";
            case Cond::LE:
                return "le";
            case Cond::AL:
                return "";
            case Cond::NV:
                break;
        }
        assert(false);
        return "<internal error>";
    }

    template<typename T>
    const char* SignStr(T value) {
        return value >= 0 ? "+" : "-";
    }

    const char* RegStr(Reg reg) {
        switch (reg) {
            case Reg::R0:
                return "r0";
            case Reg::R1:
                return "r1";
            case Reg::R2:
                return "r2";
            case Reg::R3:
                return "r3";
            case Reg::R4:
                return "r4";
            case Reg::R5:
                return "r5";
            case Reg::R6:
                return "r6";
            case Reg::R7:
                return "r7";
            case Reg::R8:
                return "r8";
            case Reg::R9:
                return "r9";
            case Reg::R10:
                return "r10";
            case Reg::R11:
                return "r11";
            case Reg::R12:
                return "r12";
            case Reg::R13:
                return "sp";
            case Reg::R14:
                return "lr";
            case Reg::R15:
                return "pc";
            case Reg::INVALID_REG:
                break;
        }
        assert(false);
        return "<internal error>";
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
                return Common::StringFromFormat("%s, LSL %s", RegStr(m), RegStr(s));
            case ShiftType::LSR:
                return Common::StringFromFormat("%s, LSR %s", RegStr(m), RegStr(s));
            case ShiftType::ASR:
                return Common::StringFromFormat("%s, ASR %s", RegStr(m), RegStr(s));
            case ShiftType::ROR:
                return Common::StringFromFormat("%s, ROR %s", RegStr(m), RegStr(s));
        }
        assert(false);
        return "<internal error>";
    }

    // Branch instructions
    std::string arm_B(Cond cond, Imm24 imm24) {
        s32 offset = Common::SignExtend<26, s32>(imm24 << 2) + 8;
        return Common::StringFromFormat("b%s %s#%i", CondStr(cond), SignStr(offset), abs(offset));
    }
    std::string arm_BL(Cond cond, Imm24 imm24) {
        s32 offset = Common::SignExtend<26, s32>(imm24 << 2) + 8;
        return Common::StringFromFormat("bl%s %s#%i", CondStr(cond), SignStr(offset), abs(offset));
    }
    std::string arm_BLX_imm(bool H, Imm24 imm24) {
        s32 offset = Common::SignExtend<26, s32>(imm24 << 2) + 8 + (H ? 2 : 0);
        return Common::StringFromFormat("blx %s#%i", SignStr(offset), abs(offset));
    }
    std::string arm_BLX_reg(Cond cond, Reg m) {
        return Common::StringFromFormat("blx%s %s", CondStr(cond), RegStr(m));
    }
    std::string arm_BX(Cond cond, Reg m) {
        return Common::StringFromFormat("bx%s %s", CondStr(cond), RegStr(m));
    }
    std::string arm_BXJ(Cond cond, Reg m) {
        return Common::StringFromFormat("bxj%s %s", CondStr(cond), RegStr(m));
    }

    // Coprocessor instructions
    std::string arm_CDP() { return "<unimplemented>"; }
    std::string arm_LDC() { return "<unimplemented>"; }
    std::string arm_MCR() { return "<unimplemented>"; }
    std::string arm_MCRR() { return "<unimplemented>"; }
    std::string arm_MRC() { return "<unimplemented>"; }
    std::string arm_MRRC() { return "<unimplemented>"; }
    std::string arm_STC() { return "<unimplemented>"; }

    // Data processing instructions
    std::string arm_ADC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("adc%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_ADC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("adc%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_ADC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("adc%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_ADD_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("add%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_ADD_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("add%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_ADD_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("add%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_AND_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("and%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_AND_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("and%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_AND_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("and%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_BIC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("bic%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_BIC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("bic%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_BIC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("bic%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_CMN_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("cmn%s %s, #%i", CondStr(cond), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_CMN_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("cmn%s %s, %s%s", CondStr(cond), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_CMN_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("cmn%s %s, %s", CondStr(cond), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_CMP_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("cmp%s %s, #%i", CondStr(cond), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_CMP_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("cmp%s %s, %s%s", CondStr(cond), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_CMP_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("cmp%s %s, %s", CondStr(cond), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_EOR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("eor%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_EOR_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("eor%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_EOR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("eor%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_MOV_imm(Cond cond, bool S, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("mov%s%s %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), ArmExpandImm(rotate, imm8));
    }
    std::string arm_MOV_reg(Cond cond, bool S, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("mov%s%s %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_MOV_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("mov%s%s %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RsrStr(s, shift, m).c_str());
    }
    std::string arm_MVN_imm(Cond cond, bool S, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("mvn%s%s %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), ArmExpandImm(rotate, imm8));
    }
    std::string arm_MVN_reg(Cond cond, bool S, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("mvn%s%s %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_MVN_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("mvn%s%s %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RsrStr(s, shift, m).c_str());
    }
    std::string arm_ORR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("orr%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_ORR_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("orr%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_ORR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("orr%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_RSB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("rsb%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_RSB_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("rsb%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_RSB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("rsb%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_RSC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("rsc%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_RSC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("rsc%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_RSC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("rsc%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_SBC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("sbc%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_SBC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("sbc%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_SBC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("sbc%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_SUB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("sub%s%s %s, %s, #%i", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_SUB_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("sub%s%s %s, %s, %s%s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_SUB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("sub%s%s %s, %s, %s", CondStr(cond), S ? "s" : "", RegStr(d), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_TEQ_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("teq%s %s, #%i", CondStr(cond), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_TEQ_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("teq%s %s, %s%s", CondStr(cond), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_TEQ_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("teq%s %s, %s", CondStr(cond), RegStr(n), RsrStr(s, shift, m).c_str());
    }
    std::string arm_TST_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return Common::StringFromFormat("tst%s %s, #%i", CondStr(cond), RegStr(n), ArmExpandImm(rotate, imm8));
    }
    std::string arm_TST_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return Common::StringFromFormat("tst%s %s, %s%s", CondStr(cond), RegStr(n), RegStr(m), ShiftStr(shift, imm5).c_str());
    }
    std::string arm_TST_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return Common::StringFromFormat("tst%s %s, %s", CondStr(cond), RegStr(n), RsrStr(s, shift, m).c_str());
    }

    // Exception generation instructions
    std::string arm_BKPT(Cond cond, Imm12 imm12, Imm4 imm4) { return "ice"; }
    std::string arm_SVC(Cond cond, Imm24 imm24) { return "ice"; }
    std::string arm_UDF() { return "ice"; }

    // Extension functions
    std::string arm_SXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_SXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_SXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_SXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_SXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_SXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_UXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_UXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_UXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_UXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_UXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }
    std::string arm_UXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) { return "ice"; }

    // Hint instructions
    std::string arm_PLD() { return "<unimplemented>"; }
    std::string arm_SEV() { return "<unimplemented>"; }
    std::string arm_WFE() { return "<unimplemented>"; }
    std::string arm_WFI() { return "<unimplemented>"; }
    std::string arm_YIELD() { return "<unimplemented>"; }

    // Load/Store instructions
    std::string arm_LDR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm12 imm12) { return "ice"; }
    std::string arm_LDR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) { return "ice"; }
    std::string arm_LDRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm12 imm12) { return "ice"; }
    std::string arm_LDRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) { return "ice"; }
    std::string arm_LDRBT() { return "ice"; }
    std::string arm_LDRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) { return "ice"; }
    std::string arm_LDRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_LDRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) { return "ice"; }
    std::string arm_LDRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_LDRHT() { return "ice"; }
    std::string arm_LDRSB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) { return "ice"; }
    std::string arm_LDRSB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_LDRSBT() { return "ice"; }
    std::string arm_LDRSH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) { return "ice"; }
    std::string arm_LDRSH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_LDRSHT() { return "ice"; }
    std::string arm_LDRT() { return "ice"; }
    std::string arm_STR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm12 imm12) { return "ice"; }
    std::string arm_STR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) { return "ice"; }
    std::string arm_STRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm12 imm12) { return "ice"; }
    std::string arm_STRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) { return "ice"; }
    std::string arm_STRBT() { return "ice"; }
    std::string arm_STRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) { return "ice"; }
    std::string arm_STRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_STRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Imm4 imm8a, Imm4 imm8b) { return "ice"; }
    std::string arm_STRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_STRHT() { return "ice"; }
    std::string arm_STRT() { return "ice"; }

    // Load/Store multiple instructions
    std::string arm_LDM(Cond cond, bool P, bool U, bool W, Reg n, RegList list) { return "ice"; }
    std::string arm_LDM_usr() { return "ice"; }
    std::string arm_LDM_eret() { return "ice"; }
    std::string arm_STM(Cond cond, bool P, bool U, bool W, Reg n, RegList list) { return "ice"; }
    std::string arm_STM_usr() { return "ice"; }

    // Miscellaneous instructions
    std::string arm_CLZ(Cond cond, Reg d, Reg m) { return "ice"; }
    std::string arm_NOP() { return "ice"; }
    std::string arm_SEL(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }

    // Unsigned sum of absolute difference functions
    std::string arm_USAD8(Cond cond, Reg d, Reg m, Reg n) { return "ice"; }
    std::string arm_USADA8(Cond cond, Reg d, Reg a, Reg m, Reg n) { return "ice"; }

    // Packing instructions
    std::string arm_PKHBT(Cond cond, Reg n, Reg d, Imm5 imm5, Reg m) { return "ice"; }
    std::string arm_PKHTB(Cond cond, Reg n, Reg d, Imm5 imm5, Reg m) { return "ice"; }

    // Reversal instructions
    std::string arm_REV(Cond cond, Reg d, Reg m) { return "ice"; }
    std::string arm_REV16(Cond cond, Reg d, Reg m) { return "ice"; }
    std::string arm_REVSH(Cond cond, Reg d, Reg m) { return "ice"; }

    // Saturation instructions
    std::string arm_SSAT(Cond cond, Imm5 sat_imm, Reg d, Imm5 imm5, bool sh, Reg n) { return "ice"; }
    std::string arm_SSAT16(Cond cond, Imm4 sat_imm, Reg d, Reg n) { return "ice"; }
    std::string arm_USAT(Cond cond, Imm5 sat_imm, Reg d, Imm5 imm5, bool sh, Reg n) { return "ice"; }
    std::string arm_USAT16(Cond cond, Imm4 sat_imm, Reg d, Reg n) { return "ice"; }

    // Multiply (Normal) instructions
    std::string arm_MLA(Cond cond, bool S, Reg d, Reg a, Reg m, Reg n) { return "ice"; }
    std::string arm_MUL(Cond cond, bool S, Reg d, Reg m, Reg n) { return "ice"; }

    // Multiply (Long) instructions
    std::string arm_SMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) { return "ice"; }
    std::string arm_SMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) { return "ice"; }
    std::string arm_UMAAL(Cond cond, Reg dHi, Reg dLo, Reg m, Reg n) { return "ice"; }
    std::string arm_UMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) { return "ice"; }
    std::string arm_UMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) { return "ice"; }

    // Multiply (Halfword) instructions
    std::string arm_SMLALxy(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, bool N, Reg n) { return "ice"; }
    std::string arm_SMLAxy(Cond cond, Reg d, Reg a, Reg m, bool M, bool N, Reg n) { return "ice"; }
    std::string arm_SMULxy(Cond cond, Reg d, Reg m, bool M, bool N, Reg n) { return "ice"; }

    // Multiply (word by halfword) instructions
    std::string arm_SMLAWy(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) { return "ice"; }
    std::string arm_SMULWy(Cond cond, Reg d, Reg m, bool M, Reg n) { return "ice"; }

    // Multiply (Most significant word) instructions
    std::string arm_SMMLA(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) { return "ice"; }
    std::string arm_SMMLS(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) { return "ice"; }
    std::string arm_SMMUL(Cond cond, Reg d, Reg m, bool R, Reg n) { return "ice"; }

    // Multiply (Dual) instructions
    std::string arm_SMLAD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) { return "ice"; }
    std::string arm_SMLALD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) { return "ice"; }
    std::string arm_SMLSD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) { return "ice"; }
    std::string arm_SMLSLD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) { return "ice"; }
    std::string arm_SMUAD(Cond cond, Reg d, Reg m, bool M, Reg n) { return "ice"; }
    std::string arm_SMUSD(Cond cond, Reg d, Reg m, bool M, Reg n) { return "ice"; }

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
    std::string arm_QADD8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QADD16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QASX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QSAX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QSUB8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_QSUB16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UQADD8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UQADD16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UQASX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UQSAX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UQSUB8(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_UQSUB16(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }

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
    std::string arm_CLREX() { return "ice"; }
    std::string arm_LDREX(Cond cond, Reg n, Reg d) { return "ice"; }
    std::string arm_LDREXB(Cond cond, Reg n, Reg d) { return "ice"; }
    std::string arm_LDREXD(Cond cond, Reg n, Reg d) { return "ice"; }
    std::string arm_LDREXH(Cond cond, Reg n, Reg d) { return "ice"; }
    std::string arm_STREX(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_STREXB(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_STREXD(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_STREXH(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SWP(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }
    std::string arm_SWPB(Cond cond, Reg n, Reg d, Reg m) { return "ice"; }

    // Status register access instructions
    std::string arm_CPS() { return "ice"; }
    std::string arm_MRS() { return "ice"; }
    std::string arm_MSR() { return "ice"; }
    std::string arm_RFE() { return "ice"; }
    std::string arm_SETEND(bool E) { return "ice"; }
    std::string arm_SRS() { return "ice"; }
};

std::string DisassembleArm(u32 instruction) {
    DisassemblerVisitor visitor;
    auto decoder = DecodeArm<DisassemblerVisitor>(instruction);
    return !decoder ? "UNKNOWN" : decoder->call(visitor, instruction);
}

} // namespace Arm
} // namespace Dynarmic
