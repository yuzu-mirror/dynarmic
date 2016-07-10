/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <tuple>

#include "common/assert.h"
#include "frontend/arm_types.h"
#include "frontend/decoder/thumb1.h"
#include "frontend/ir_emitter.h"
#include "frontend/translate.h"

namespace Dynarmic {
namespace Arm {

struct TranslatorVisitor final {
    explicit TranslatorVisitor(LocationDescriptor descriptor) : ir(descriptor) {
        ASSERT_MSG(descriptor.TFlag, "The processor must be in Thumb mode");
    }

    IREmitter ir;

    bool TranslateThisInstruction() {
        ir.SetTerm(IR::Term::Interpret(ir.current_location));
        return false;
    }
    bool UnpredictableInstruction() {
        ASSERT_MSG(false, "UNPREDICTABLE");
        return false;
    }

    bool thumb1_LSL_imm(Imm5 imm5, Reg m, Reg d) {
        u8 shift_n = imm5;
        // LSLS <Rd>, <Rm>, #<imm5>
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.LogicalShiftLeft(ir.GetRegister(m), ir.Imm8(shift_n), cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        return true;
    }
    bool thumb1_LSR_imm(Imm5 imm5, Reg m, Reg d) {
        u8 shift_n = imm5 != 0 ? imm5 : 32;
        // LSRS <Rd>, <Rm>, #<imm5>
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.LogicalShiftRight(ir.GetRegister(m), ir.Imm8(shift_n), cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        return true;
    }
    bool thumb1_ASR_imm(Imm5 imm5, Reg m, Reg d) {
        u8 shift_n = imm5 != 0 ? imm5 : 32;
        // ASRS <Rd>, <Rm>, #<imm5>
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.ArithmeticShiftRight(ir.GetRegister(m), ir.Imm8(shift_n), cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        return true;
    }
    bool thumb1_ADD_reg_t1(Reg m, Reg n, Reg d) {
        // ADDS <Rd>, <Rn>, <Rm>
        // Note that it is not possible to encode Rd == R15.
        auto result = ir.AddWithCarry(ir.GetRegister(n), ir.GetRegister(m), ir.Imm1(0));
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
        return true;
    }
    bool thumb1_SUB_reg(Reg m, Reg n, Reg d) {
        // SUBS <Rd>, <Rn>, <Rm>
        // Note that it is not possible to encode Rd == R15.
        auto result = ir.SubWithCarry(ir.GetRegister(n), ir.GetRegister(m), ir.Imm1(1));
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
        return true;
    }
    bool thumb1_ADD_imm_t1(Imm3 imm3, Reg n, Reg d) {
        u32 imm32 = imm3 & 0x7;
        // ADDS <Rd>, <Rn>, #<imm3>
        // Rd can never encode R15.
        auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(0));
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
        return true;
    }
    bool thumb1_SUB_imm_t1(Imm3 imm3, Reg n, Reg d) {
        u32 imm32 = imm3 & 0x7;
        // SUBS <Rd>, <Rn>, #<imm3>
        // Rd can never encode R15.
        auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
        return true;
    }
    bool thumb1_MOV_imm(Reg d, Imm8 imm8) {
        u32 imm32 = imm8 & 0xFF;
        // MOVS <Rd>, #<imm8>
        // Rd can never encode R15.
        auto result = ir.Imm32(imm32);
        ir.SetRegister(d, result);
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        return true;
    }
    bool thumb1_CMP_imm(Reg n, Imm8 imm8) {
        u32 imm32 = imm8 & 0xFF;
        // CMP <Rn>, #<imm8>
        auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
        return true;
    }
    bool thumb1_ADD_imm_t2(Reg d_n, Imm8 imm8) {
        u32 imm32 = imm8 & 0xFF;
        Reg d = d_n, n = d_n;
        // ADDS <Rdn>, #<imm8>
        // Rd can never encode R15.
        auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(0));
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
        return true;
    }
    bool thumb1_SUB_imm_t2(Reg d_n, Imm8 imm8) {
        u32 imm32 = imm8 & 0xFF;
        Reg d = d_n, n = d_n;
        // SUBS <Rd>, <Rn>, #<imm3>
        // Rd can never encode R15.
        auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
        return true;
    }

    bool thumb1_AND_reg(Reg m, Reg d_n) {
        const Reg d = d_n, n = d_n;
        // ANDS <Rdn>, <Rm>
        // Note that it is not possible to encode Rdn == R15.
        auto result = ir.And(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        return true;
    }
    bool thumb1_EOR_reg(Reg m, Reg d_n) {
        const Reg d = d_n, n = d_n;
        // EORS <Rdn>, <Rm>
        // Note that it is not possible to encode Rdn == R15.
        auto result = ir.Eor(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetRegister(d, result);
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        return true;
    }
    bool thumb1_LSL_reg(Reg m, Reg d_n) {
        const Reg d = d_n, n = d_n;
        // LSLS <Rdn>, <Rm>
        auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
        auto apsr_c = ir.GetCFlag();
        auto result_carry = ir.LogicalShiftLeft(ir.GetRegister(n), shift_n, apsr_c);
        ir.SetRegister(d, result_carry.result);
        ir.SetNFlag(ir.MostSignificantBit(result_carry.result));
        ir.SetZFlag(ir.IsZero(result_carry.result));
        ir.SetCFlag(result_carry.carry);
        return true;
    }
    bool thumb1_LSR_reg(Reg m, Reg d_n) {
        const Reg d = d_n, n = d_n;
        // LSRS <Rdn>, <Rm>
        auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.LogicalShiftRight(ir.GetRegister(n), shift_n, cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        return true;
    }
    bool thumb1_ASR_reg(Reg m, Reg d_n) {
        const Reg d = d_n, n = d_n;
        // ASRS <Rdn>, <Rm>
        auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.ArithmeticShiftRight(ir.GetRegister(n), shift_n, cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        return true;
    }
    bool thumb1_ADC_reg(Reg m, Reg d_n) {
        Reg d = d_n, n = d_n;
        // ADCS <Rdn>, <Rm>
        // Note that it is not possible to encode Rd == R15.
        auto aspr_c = ir.GetCFlag();
        auto result = ir.AddWithCarry(ir.GetRegister(n), ir.GetRegister(m), aspr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
        return true;
    }
    bool thumb1_SBC_reg(Reg m, Reg d_n) {
        Reg d = d_n, n = d_n;
        // SBCS <Rdn>, <Rm>
        // Note that it is not possible to encode Rd == R15.
        auto aspr_c = ir.GetCFlag();
        auto result = ir.SubWithCarry(ir.GetRegister(n), ir.GetRegister(m), aspr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
        return true;
    }
    bool thumb1_ROR_reg(Reg m, Reg d_n) {
        Reg d = d_n, n = d_n;
        // RORS <Rdn>, <Rm>
        auto shift_n = ir.LeastSignificantByte(ir.GetRegister(m));
        auto cpsr_c = ir.GetCFlag();
        auto result = ir.RotateRight(ir.GetRegister(n), shift_n, cpsr_c);
        ir.SetRegister(d, result.result);
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        return true;
    }
    bool thumb1_TST_reg(Reg m, Reg n) {
        // TST <Rn>, <Rm>
        auto result = ir.And(ir.GetRegister(n), ir.GetRegister(m));
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        return true;
    }

    bool thumb1_ADD_reg_t2(bool d_n_hi, Reg m, Reg d_n_lo) {
        Reg d_n = d_n_hi ? (d_n_lo + 8) : d_n_lo;
        Reg d = d_n, n = d_n;
        if (n == Reg::PC && m == Reg::PC) {
            return UnpredictableInstruction();
        }
        // ADD <Rdn>, <Rm>
        auto result = ir.AddWithCarry(ir.GetRegister(n), ir.GetRegister(m), ir.Imm1(0));
        if (d == Reg::PC) {
            ir.ALUWritePC(result.result);
            // Return to dispatch as we can't predict what PC is going to be. Stop compilation.
            ir.SetTerm(IR::Term::ReturnToDispatch{});
            return false;
        } else {
            ir.SetRegister(d, result.result);
            return true;
        }
    }

    bool thumb1_UDF() {
        return TranslateThisInstruction();
    }
};

enum class ThumbInstSize {
    Thumb16, Thumb32
};

static std::tuple<u32, ThumbInstSize> ReadThumbInstruction(u32 arm_pc, MemoryRead32FuncType memory_read_32) {
    u32 first_part = (*memory_read_32)(arm_pc & 0xFFFFFFFC);
    if ((arm_pc & 0x2) != 0)
        first_part >>= 16;
    first_part &= 0xFFFF;

    if ((first_part & 0xF800) <= 0xE800) {
        // 16-bit thumb instruction
        return std::make_tuple(first_part, ThumbInstSize::Thumb16);
    }

    // 32-bit thumb instruction
    // These always start with 0b11101, 0b11110 or 0b11111.

    u32 second_part = (*memory_read_32)((arm_pc+2) & 0xFFFFFFFC);
    if (((arm_pc+2) & 0x2) != 0)
        second_part >>= 16;
    second_part &= 0xFFFF;

    return std::make_tuple(static_cast<u32>((first_part << 16) | second_part), ThumbInstSize::Thumb32);
}

IR::Block TranslateThumb(LocationDescriptor descriptor, MemoryRead32FuncType memory_read_32) {
    TranslatorVisitor visitor{descriptor};

    bool should_continue = true;
    while (should_continue) {
        const u32 arm_pc = visitor.ir.current_location.arm_pc;

        u32 thumb_instruction;
        ThumbInstSize inst_size;
        std::tie(thumb_instruction, inst_size) = ReadThumbInstruction(arm_pc, memory_read_32);

        if (inst_size == ThumbInstSize::Thumb16) {
            auto decoder = DecodeThumb16<TranslatorVisitor>(static_cast<u16>(thumb_instruction));
            if (decoder) {
                should_continue = decoder->call(visitor, static_cast<u16>(thumb_instruction));
            } else {
                should_continue = visitor.thumb1_UDF();
            }
        } else {
            /*auto decoder = DecodeThumb2<TranslatorVisitor>(thumb_instruction);
            if (decoder) {
                should_continue = decoder->call(visitor, thumb_instruction);
            } else {
                should_continue = visitor.thumb2_UDF();
            }*/
            ASSERT_MSG(0, "Unimplemented");
        }

        visitor.ir.current_location.arm_pc += (inst_size == ThumbInstSize::Thumb16) ? 2 : 4;
        visitor.ir.block.cycle_count++;
    }

    return visitor.ir.block;
}

} // namespace Arm
} // namepsace Dynarmic
