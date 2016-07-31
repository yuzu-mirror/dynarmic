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

void IREmitter::SetRegister(const Reg reg, const IR::Value& value) {
    ASSERT(reg != Reg::PC);
    Inst(IR::Opcode::SetRegister, { IR::Value(reg), value });
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

IR::Value IREmitter::SignExtendHalfToWord(const IR::Value& a) {
    return Inst(IR::Opcode::SignExtendHalfToWord, {a});
}

IR::Value IREmitter::SignExtendByteToWord(const IR::Value& a) {
    return Inst(IR::Opcode::SignExtendByteToWord, {a});
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

void IREmitter::SetTerm(const IR::Terminal& terminal) {
    ASSERT_MSG(block.terminal.which() == 0, "Terminal has already been set.");
    block.terminal = terminal;
}

IR::Value IREmitter::Inst(IR::Opcode op, std::initializer_list<IR::Value> args) {
    IR::Inst* inst = new(block.instruction_alloc_pool->malloc()) IR::Inst(op);
    DEBUG_ASSERT(args.size() == inst->NumArgs());

    std::for_each(args.begin(), args.end(), [&inst, op, index = size_t(0)](const auto& v) mutable {
        DEBUG_ASSERT(IR::GetArgTypeOf(op, index) == v.GetType());
        inst->SetArg(index, v);
        index++;
    });

    block.instructions.push_back(*inst);
    return IR::Value(inst);
}

} // namespace Arm
} // namespace Dynarmic

