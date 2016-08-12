/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "ir_emitter.h"

namespace Dynarmic {
namespace Arm {

void IREmitter::Unimplemented() {

}

u32 IREmitter::PC() {
    u32 offset = current_location.TFlag() ? 4 : 8;
    return current_location.PC() + offset;
}

u32 IREmitter::AlignPC(size_t alignment) {
    u32 pc = PC();
    return static_cast<u32>(pc - pc % alignment);
}

IR::Value IREmitter::Imm1(bool imm1) {
    return IR::Value(imm1);
}

IR::Value IREmitter::Imm8(u8 imm8) {
    return IR::Value(imm8);
}

IR::Value IREmitter::Imm32(u32 imm32) {
    return IR::Value(imm32);
}

IR::Value IREmitter::GetRegister(Reg reg) {
    if (reg == Reg::PC) {
        return Imm32(PC());
    }
    return Inst(IR::Opcode::GetRegister, { IR::Value(reg) });
}

IR::Value IREmitter::GetExtendedRegister(ExtReg reg) {
    if (reg >= Arm::ExtReg::S0 && reg <= Arm::ExtReg::S31) {
        return Inst(IR::Opcode::GetExtendedRegister32, {IR::Value(reg)});
    } else if (reg >= Arm::ExtReg::D0 && reg <= Arm::ExtReg::D31) {
        return Inst(IR::Opcode::GetExtendedRegister64, {IR::Value(reg)});
    } else {
        ASSERT_MSG(false, "Invalid reg.");
    }
}

void IREmitter::SetRegister(const Reg reg, const IR::Value& value) {
    ASSERT(reg != Reg::PC);
    Inst(IR::Opcode::SetRegister, { IR::Value(reg), value });
}

void IREmitter::SetExtendedRegister(const ExtReg reg, const IR::Value& value) {
    if (reg >= Arm::ExtReg::S0 && reg <= Arm::ExtReg::S31) {
        Inst(IR::Opcode::SetExtendedRegister32, {IR::Value(reg), value});
    } else if (reg >= Arm::ExtReg::D0 && reg <= Arm::ExtReg::D31) {
        Inst(IR::Opcode::SetExtendedRegister64, {IR::Value(reg), value});
    } else {
        ASSERT_MSG(false, "Invalid reg.");
    }
}

void IREmitter::ALUWritePC(const IR::Value& value) {
    // This behaviour is ARM version-dependent.
    // The below implementation is for ARMv6k
    BranchWritePC(value);
}

void IREmitter::BranchWritePC(const IR::Value& value) {
    if (!current_location.TFlag()) {
        auto new_pc = And(value, Imm32(0xFFFFFFFC));
        Inst(IR::Opcode::SetRegister, { IR::Value(Reg::PC), new_pc });
    } else {
        auto new_pc = And(value, Imm32(0xFFFFFFFE));
        Inst(IR::Opcode::SetRegister, { IR::Value(Reg::PC), new_pc });
    }
}

void IREmitter::BXWritePC(const IR::Value& value) {
    Inst(IR::Opcode::BXWritePC, {value});
}

void IREmitter::LoadWritePC(const IR::Value& value) {
    // This behaviour is ARM version-dependent.
    // The below implementation is for ARMv6k
    BXWritePC(value);
}

void IREmitter::CallSupervisor(const IR::Value& value) {
    Inst(IR::Opcode::CallSupervisor, {value});
}

IR::Value IREmitter::GetCFlag() {
    return Inst(IR::Opcode::GetCFlag, {});
}

void IREmitter::SetNFlag(const IR::Value& value) {
    Inst(IR::Opcode::SetNFlag, {value});
}

void IREmitter::SetZFlag(const IR::Value& value) {
    Inst(IR::Opcode::SetZFlag, {value});
}

void IREmitter::SetCFlag(const IR::Value& value) {
    Inst(IR::Opcode::SetCFlag, {value});
}

void IREmitter::SetVFlag(const IR::Value& value) {
    Inst(IR::Opcode::SetVFlag, {value});
}

void IREmitter::OrQFlag(const IR::Value& value) {
    Inst(IR::Opcode::OrQFlag, {value});
}

IR::Value IREmitter::Pack2x32To1x64(const IR::Value& lo, const IR::Value& hi)
{
    return Inst(IR::Opcode::Pack2x32To1x64, {lo, hi});
}

IR::Value IREmitter::LeastSignificantWord(const IR::Value& value) {
    return Inst(IR::Opcode::LeastSignificantWord, {value});
}

IREmitter::ResultAndCarry IREmitter::MostSignificantWord(const IR::Value& value) {
    auto result = Inst(IR::Opcode::MostSignificantWord, {value});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IR::Value IREmitter::LeastSignificantHalf(const IR::Value& value) {
    return Inst(IR::Opcode::LeastSignificantHalf, {value});
}

IR::Value IREmitter::LeastSignificantByte(const IR::Value& value) {
    return Inst(IR::Opcode::LeastSignificantByte, {value});
}

IR::Value IREmitter::MostSignificantBit(const IR::Value& value) {
    return Inst(IR::Opcode::MostSignificantBit, {value});
}

IR::Value IREmitter::IsZero(const IR::Value& value) {
    return Inst(IR::Opcode::IsZero, {value});
}

IR::Value IREmitter::IsZero64(const IR::Value& value) {
    return Inst(IR::Opcode::IsZero64, {value});
}

IREmitter::ResultAndCarry IREmitter::LogicalShiftLeft(const IR::Value& value_in, const IR::Value& shift_amount, const IR::Value& carry_in) {
    auto result = Inst(IR::Opcode::LogicalShiftLeft, {value_in, shift_amount, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IREmitter::ResultAndCarry IREmitter::LogicalShiftRight(const IR::Value& value_in, const IR::Value& shift_amount, const IR::Value& carry_in) {
    auto result = Inst(IR::Opcode::LogicalShiftRight, {value_in, shift_amount, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IR::Value IREmitter::LogicalShiftRight64(const IR::Value& value_in, const IR::Value& shift_amount) {
    return Inst(IR::Opcode::LogicalShiftRight64, {value_in, shift_amount});
}

IREmitter::ResultAndCarry IREmitter::ArithmeticShiftRight(const IR::Value& value_in, const IR::Value& shift_amount, const IR::Value& carry_in) {
    auto result = Inst(IR::Opcode::ArithmeticShiftRight, {value_in, shift_amount, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IREmitter::ResultAndCarry IREmitter::RotateRight(const IR::Value& value_in, const IR::Value& shift_amount, const IR::Value& carry_in) {
    auto result = Inst(IR::Opcode::RotateRight, {value_in, shift_amount, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IREmitter::ResultAndCarry IREmitter::RotateRightExtended(const IR::Value& value_in, const IR::Value& carry_in) {
    auto result = Inst(IR::Opcode::RotateRightExtended, {value_in, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IREmitter::ResultAndCarryAndOverflow IREmitter::AddWithCarry(const IR::Value& a, const IR::Value& b, const IR::Value& carry_in) {
    auto result = Inst(IR::Opcode::AddWithCarry, {a, b, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    auto overflow = Inst(IR::Opcode::GetOverflowFromOp, {result});
    return {result, carry_out, overflow};
}

IR::Value IREmitter::Add(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::AddWithCarry, {a, b, Imm1(0)});
}

IR::Value IREmitter::Add64(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::Add64, {a, b});
}

IREmitter::ResultAndCarryAndOverflow IREmitter::SubWithCarry(const IR::Value& a, const IR::Value& b, const IR::Value& carry_in) {
    // This is equivalent to AddWithCarry(a, Not(b), carry_in).
    auto result = Inst(IR::Opcode::SubWithCarry, {a, b, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    auto overflow = Inst(IR::Opcode::GetOverflowFromOp, {result});
    return {result, carry_out, overflow};
}

IR::Value IREmitter::Sub(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::SubWithCarry, {a, b, Imm1(1)});
}

IR::Value IREmitter::Sub64(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::Sub64, {a, b});
}

IR::Value IREmitter::Mul(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::Mul, {a, b});
}

IR::Value IREmitter::Mul64(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::Mul64, {a, b});
}

IR::Value IREmitter::And(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::And, {a, b});
}

IR::Value IREmitter::Eor(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::Eor, {a, b});
}

IR::Value IREmitter::Or(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::Or, {a, b});
}

IR::Value IREmitter::Not(const IR::Value& a) {
    return Inst(IR::Opcode::Not, {a});
}

IR::Value IREmitter::SignExtendWordToLong(const IR::Value& a) {
    return Inst(IR::Opcode::SignExtendWordToLong, {a});
}

IR::Value IREmitter::SignExtendHalfToWord(const IR::Value& a) {
    return Inst(IR::Opcode::SignExtendHalfToWord, {a});
}

IR::Value IREmitter::SignExtendByteToWord(const IR::Value& a) {
    return Inst(IR::Opcode::SignExtendByteToWord, {a});
}

IR::Value IREmitter::ZeroExtendWordToLong(const IR::Value& a) {
    return Inst(IR::Opcode::ZeroExtendWordToLong, {a});
}

IR::Value IREmitter::ZeroExtendHalfToWord(const IR::Value& a) {
    return Inst(IR::Opcode::ZeroExtendHalfToWord, {a});
}

IR::Value IREmitter::ZeroExtendByteToWord(const IR::Value& a) {
    return Inst(IR::Opcode::ZeroExtendByteToWord, {a});
}

IR::Value IREmitter::ByteReverseWord(const IR::Value& a) {
    return Inst(IR::Opcode::ByteReverseWord, {a});
}

IR::Value IREmitter::ByteReverseHalf(const IR::Value& a) {
    return Inst(IR::Opcode::ByteReverseHalf, {a});
}

IR::Value IREmitter::ByteReverseDual(const IR::Value& a) {
    return Inst(IR::Opcode::ByteReverseDual, {a});
}

IR::Value IREmitter::PackedSaturatedAddU8(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::PackedSaturatedAddU8, {a, b});
}

IR::Value IREmitter::PackedSaturatedAddS8(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::PackedSaturatedAddS8, {a, b});
}

IR::Value IREmitter::PackedSaturatedSubU8(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::PackedSaturatedSubU8, {a, b});
}

IR::Value IREmitter::PackedSaturatedSubS8(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::PackedSaturatedSubS8, {a, b});
}

IR::Value IREmitter::PackedSaturatedAddU16(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::PackedSaturatedAddU16, {a, b});
}

IR::Value IREmitter::PackedSaturatedAddS16(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::PackedSaturatedAddS16, {a, b});
}

IR::Value IREmitter::PackedSaturatedSubU16(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::PackedSaturatedSubU16, {a, b});
}

IR::Value IREmitter::PackedSaturatedSubS16(const IR::Value& a, const IR::Value& b) {
    return Inst(IR::Opcode::PackedSaturatedSubS16, {a, b});
}

IR::Value IREmitter::TransferToFP32(const IR::Value& a) {
    return Inst(IR::Opcode::TransferToFP32, {a});
}

IR::Value IREmitter::TransferToFP64(const IR::Value& a) {
    return Inst(IR::Opcode::TransferToFP64, {a});
}

IR::Value IREmitter::TransferFromFP32(const IR::Value& a) {
    return Inst(IR::Opcode::TransferFromFP32, {a});
}

IR::Value IREmitter::TransferFromFP64(const IR::Value& a) {
    return Inst(IR::Opcode::TransferFromFP64, {a});
}

IR::Value IREmitter::FPAbs32(const IR::Value& a) {
    return Inst(IR::Opcode::FPAbs32, {a});
}

IR::Value IREmitter::FPAbs64(const IR::Value& a) {
    return Inst(IR::Opcode::FPAbs64, {a});
}

IR::Value IREmitter::FPAdd32(const IR::Value& a, const IR::Value& b, bool fpscr_controlled) {
    ASSERT(fpscr_controlled);
    return Inst(IR::Opcode::FPAdd32, {a, b});
}

IR::Value IREmitter::FPAdd64(const IR::Value& a, const IR::Value& b, bool fpscr_controlled) {
    ASSERT(fpscr_controlled);
    return Inst(IR::Opcode::FPAdd64, {a, b});
}

IR::Value IREmitter::FPDiv32(const IR::Value& a, const IR::Value& b, bool fpscr_controlled) {
    ASSERT(fpscr_controlled);
    return Inst(IR::Opcode::FPDiv32, {a, b});
}

IR::Value IREmitter::FPDiv64(const IR::Value& a, const IR::Value& b, bool fpscr_controlled) {
    ASSERT(fpscr_controlled);
    return Inst(IR::Opcode::FPDiv64, {a, b});
}

IR::Value IREmitter::FPMul32(const IR::Value& a, const IR::Value& b, bool fpscr_controlled) {
    ASSERT(fpscr_controlled);
    return Inst(IR::Opcode::FPMul32, {a, b});
}

IR::Value IREmitter::FPMul64(const IR::Value& a, const IR::Value& b, bool fpscr_controlled) {
    ASSERT(fpscr_controlled);
    return Inst(IR::Opcode::FPMul64, {a, b});
}

IR::Value IREmitter::FPNeg32(const IR::Value& a) {
    return Inst(IR::Opcode::FPNeg32, {a});
}

IR::Value IREmitter::FPNeg64(const IR::Value& a) {
    return Inst(IR::Opcode::FPNeg64, {a});
}

IR::Value IREmitter::FPSqrt32(const IR::Value& a) {
    return Inst(IR::Opcode::FPSqrt32, {a});
}

IR::Value IREmitter::FPSqrt64(const IR::Value& a) {
    return Inst(IR::Opcode::FPSqrt64, {a});
}

IR::Value IREmitter::FPSub32(const IR::Value& a, const IR::Value& b, bool fpscr_controlled) {
    ASSERT(fpscr_controlled);
    return Inst(IR::Opcode::FPSub32, {a, b});
}

IR::Value IREmitter::FPSub64(const IR::Value& a, const IR::Value& b, bool fpscr_controlled) {
    ASSERT(fpscr_controlled);
    return Inst(IR::Opcode::FPSub64, {a, b});
}

void IREmitter::ClearExlcusive() {
    Inst(IR::Opcode::ClearExclusive, {});
}

void IREmitter::SetExclusive(const IR::Value& vaddr, size_t byte_size) {
    ASSERT(byte_size == 1 || byte_size == 2 || byte_size == 4 || byte_size == 8 || byte_size == 16);
    Inst(IR::Opcode::SetExclusive, {vaddr, Imm8(u8(byte_size))});
}

IR::Value IREmitter::ReadMemory8(const IR::Value& vaddr) {
    return Inst(IR::Opcode::ReadMemory8, {vaddr});
}

IR::Value IREmitter::ReadMemory16(const IR::Value& vaddr) {
    auto value = Inst(IR::Opcode::ReadMemory16, {vaddr});
    return current_location.EFlag() ? ByteReverseHalf(value) : value;
}

IR::Value IREmitter::ReadMemory32(const IR::Value& vaddr) {
    auto value = Inst(IR::Opcode::ReadMemory32, {vaddr});
    return current_location.EFlag() ? ByteReverseWord(value) : value;
}

IR::Value IREmitter::ReadMemory64(const IR::Value& vaddr) {
    auto value = Inst(IR::Opcode::ReadMemory64, {vaddr});
    return current_location.EFlag() ? ByteReverseDual(value) : value;
}

void IREmitter::WriteMemory8(const IR::Value& vaddr, const IR::Value& value) {
    Inst(IR::Opcode::WriteMemory8, {vaddr, value});
}

void IREmitter::WriteMemory16(const IR::Value& vaddr, const IR::Value& value) {
    if (current_location.EFlag()) {
        auto v = ByteReverseHalf(value);
        Inst(IR::Opcode::WriteMemory16, {vaddr, v});
    } else {
        Inst(IR::Opcode::WriteMemory16, {vaddr, value});
    }
}

void IREmitter::WriteMemory32(const IR::Value& vaddr, const IR::Value& value) {
    if (current_location.EFlag()) {
        auto v = ByteReverseWord(value);
        Inst(IR::Opcode::WriteMemory32, {vaddr, v});
    } else {
        Inst(IR::Opcode::WriteMemory32, {vaddr, value});
    }
}

void IREmitter::WriteMemory64(const IR::Value& vaddr, const IR::Value& value) {
    if (current_location.EFlag()) {
        auto v = ByteReverseDual(value);
        Inst(IR::Opcode::WriteMemory64, {vaddr, v});
    } else {
        Inst(IR::Opcode::WriteMemory64, {vaddr, value});
    }
}

IR::Value IREmitter::ExclusiveWriteMemory8(const IR::Value& vaddr, const IR::Value& value) {
    return Inst(IR::Opcode::ExclusiveWriteMemory8, {vaddr, value});
}

IR::Value IREmitter::ExclusiveWriteMemory16(const IR::Value& vaddr, const IR::Value& value) {
    if (current_location.EFlag()) {
        auto v = ByteReverseHalf(value);
        return Inst(IR::Opcode::ExclusiveWriteMemory16, {vaddr, v});
    } else {
        return Inst(IR::Opcode::ExclusiveWriteMemory16, {vaddr, value});
    }
}

IR::Value IREmitter::ExclusiveWriteMemory32(const IR::Value& vaddr, const IR::Value& value) {
    if (current_location.EFlag()) {
        auto v = ByteReverseWord(value);
        return Inst(IR::Opcode::ExclusiveWriteMemory32, {vaddr, v});
    } else {
        return Inst(IR::Opcode::ExclusiveWriteMemory32, {vaddr, value});
    }
}

IR::Value IREmitter::ExclusiveWriteMemory64(const IR::Value& vaddr, const IR::Value& value_lo, const IR::Value& value_hi) {
    if (current_location.EFlag()) {
        auto vlo = ByteReverseWord(value_lo);
        auto vhi = ByteReverseWord(value_hi);
        return Inst(IR::Opcode::ExclusiveWriteMemory64, {vaddr, vlo, vhi});
    } else {
        return Inst(IR::Opcode::ExclusiveWriteMemory64, {vaddr, value_lo, value_hi});
    }
}

void IREmitter::Breakpoint() {
    Inst(IR::Opcode::Breakpoint, {});
}

void IREmitter::SetTerm(const IR::Terminal& terminal) {
    ASSERT_MSG(block.terminal.which() == 0, "Terminal has already been set.");
    block.terminal = terminal;
}

IR::Value IREmitter::Inst(IR::Opcode op, std::initializer_list<IR::Value> args) {
    IR::Inst* inst = new(block.instruction_alloc_pool->Alloc()) IR::Inst(op);
    DEBUG_ASSERT(args.size() == inst->NumArgs());

    std::for_each(args.begin(), args.end(), [&inst, op, index = size_t(0)](const auto& v) mutable {
        inst->SetArg(index, v);
        index++;
    });

    block.instructions.Append(*inst);
    return IR::Value(inst);
}

} // namespace Arm
} // namespace Dynarmic

