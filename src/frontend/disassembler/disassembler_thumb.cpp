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

    std::string thumb16_LSL_imm(Imm5 imm5, Reg m, Reg d) {
        return Common::StringFromFormat("lsls %s, %s, #%u", RegStr(d), RegStr(m), imm5);
    }

    std::string thumb16_LSR_imm(Imm5 imm5, Reg m, Reg d) {
        return Common::StringFromFormat("lsrs %s, %s, #%u", RegStr(d), RegStr(m), imm5);
    }

    std::string thumb16_ASR_imm(Imm5 imm5, Reg m, Reg d) {
        return Common::StringFromFormat("asrs %s, %s, #%u", RegStr(d), RegStr(m), imm5);
    }

    std::string thumb16_ADD_reg_t1(Reg m, Reg n, Reg d) {
        return Common::StringFromFormat("adds %s, %s, %s", RegStr(d), RegStr(n), RegStr(m));
    }

    std::string thumb16_SUB_reg(Reg m, Reg n, Reg d) {
        return Common::StringFromFormat("subs %s, %s, %s", RegStr(d), RegStr(n), RegStr(m));
    }

    std::string thumb16_ADD_imm_t1(Imm3 imm3, Reg n, Reg d) {
        return Common::StringFromFormat("adds %s, %s, #%u", RegStr(d), RegStr(n), imm3);
    }

    std::string thumb16_SUB_imm_t1(Imm3 imm3, Reg n, Reg d) {
        return Common::StringFromFormat("subs %s, %s, #%u", RegStr(d), RegStr(n), imm3);
    }

    std::string thumb16_MOV_imm(Reg d, Imm8 imm8) {
        return Common::StringFromFormat("movs %s, #%u", RegStr(d), imm8);
    }

    std::string thumb16_CMP_imm(Reg n, Imm8 imm8) {
        return Common::StringFromFormat("cmp %s, #%u", RegStr(n), imm8);
    }

    std::string thumb16_ADD_imm_t2(Reg d_n, Imm8 imm8) {
        return Common::StringFromFormat("adds %s, #%u", RegStr(d_n), imm8);
    }

    std::string thumb16_SUB_imm_t2(Reg d_n, Imm8 imm8) {
        return Common::StringFromFormat("subs %s, #%u", RegStr(d_n), imm8);
    }

    std::string thumb16_AND_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("ands %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_EOR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("eors %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_LSL_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("lsls %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_LSR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("lsrs %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_ASR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("asrs %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_ADC_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("adcs %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_SBC_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("sbcs %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_ROR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("rors %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_TST_reg(Reg m, Reg n) {
        return Common::StringFromFormat("tst %s, %s", RegStr(n), RegStr(m));
    }

    std::string thumb16_RSB_imm(Reg n, Reg d) {
        // Pre-UAL syntax: NEGS <Rd>, <Rn>
        return Common::StringFromFormat("rsbs %s, %s, #0", RegStr(d), RegStr(n));
    }

    std::string thumb16_CMP_reg_t1(Reg m, Reg n) {
        return Common::StringFromFormat("cmp %s, %s", RegStr(n), RegStr(m));
    }

    std::string thumb16_CMN_reg(Reg m, Reg n) {
        return Common::StringFromFormat("cmn %s, %s", RegStr(n), RegStr(m));
    }

    std::string thumb16_ORR_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("orrs %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_BIC_reg(Reg m, Reg d_n) {
        return Common::StringFromFormat("bics %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_MVN_reg(Reg m, Reg d) {
        return Common::StringFromFormat("mvns %s, %s", RegStr(d), RegStr(m));
    }

    std::string thumb16_ADD_reg_t2(bool d_n_hi, Reg m, Reg d_n_lo) {
        Reg d_n = d_n_hi ? (d_n_lo + 8) : d_n_lo;
        return Common::StringFromFormat("add %s, %s", RegStr(d_n), RegStr(m));
    }

    std::string thumb16_CMP_reg_t2(bool n_hi, Reg m, Reg n_lo) {
        Reg n = n_hi ? (n_lo + 8) : n_lo;
        return Common::StringFromFormat("cmp %s, %s", RegStr(n), RegStr(m));
    }

    std::string thumb16_MOV_reg(bool d_hi, Reg m, Reg d_lo) {
        Reg d = d_hi ? (d_lo + 8) : d_lo;
        return Common::StringFromFormat("mov %s, %s", RegStr(d), RegStr(m));
    }

    std::string thumb16_LDR_literal(Reg t, Imm8 imm8) {
        u32 imm32 = imm8 << 2;
        return Common::StringFromFormat("ldr %s, [pc, #%u]", RegStr(t), imm32);
    }

    std::string thumb16_STR_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("str %s, [%s, %s]", RegStr(t), RegStr(n), RegStr(m));
    }

    std::string thumb16_STRH_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("strh %s, [%s, %s]", RegStr(t), RegStr(n), RegStr(m));
    }

    std::string thumb16_STRB_reg(Reg m, Reg n, Reg t) {
        return Common::StringFromFormat("strb %s, [%s, %s]", RegStr(t), RegStr(n), RegStr(m));
    }

    std::string thumb16_LDR_imm_t1(Imm5 imm5, Reg n, Reg t) {
        u32 imm32 = imm5 << 2;
        return Common::StringFromFormat("ldr %s, [%s, #%u]", RegStr(t), RegStr(n), imm32);
    }

    std::string thumb16_UDF() {
        return Common::StringFromFormat("udf");
    }

    std::string thumb16_SVC(Imm8 imm8) {
        return Common::StringFromFormat("svc #%u", imm8);
    }
};

std::string DisassembleThumb16(u16 instruction) {
    DisassemblerVisitor visitor;
    auto decoder = DecodeThumb16<DisassemblerVisitor>(instruction);
    return !decoder ? Common::StringFromFormat("UNKNOWN: %x", instruction) : decoder->call(visitor, instruction);
}

} // namespace Arm
} // namespace Dynarmic
