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
    u32 offset = current_location.TFlag ? 4 : 8;
    return current_location.arm_pc + offset;
}

u32 IREmitter::AlignPC(size_t alignment) {
    u32 pc = PC();
    return static_cast<u32>(pc - pc % alignment);
}

IR::ValuePtr IREmitter::Imm1(bool value) {
    auto imm1 = std::make_shared<IR::ImmU1>(value);
    AddToBlock(imm1);
    return imm1;
}

IR::ValuePtr IREmitter::Imm8(u8 i) {
    auto imm8 = std::make_shared<IR::ImmU8>(i);
    AddToBlock(imm8);
    return imm8;
}

IR::ValuePtr IREmitter::Imm32(u32 i) {
    auto imm32 = std::make_shared<IR::ImmU32>(i);
    AddToBlock(imm32);
    return imm32;
}

IR::ValuePtr IREmitter::GetRegister(Reg reg) {
    if (reg == Reg::PC) {
        return Imm32(PC());
    }
    return Inst(IR::Opcode::GetRegister, { RegRef(reg) });
}

void IREmitter::SetRegister(const Reg reg, IR::ValuePtr value) {
    ASSERT(reg != Reg::PC);
    Inst(IR::Opcode::SetRegister, { RegRef(reg), value });
}

void IREmitter::ALUWritePC(IR::ValuePtr value) {
    // This behaviour is ARM version-dependent.
    // The below implementation is for ARMv6k
    if (!current_location.TFlag) {
        auto new_pc = And(value, Imm32(0xFFFFFFFC));
        Inst(IR::Opcode::SetRegister, { RegRef(Reg::PC), new_pc });
    } else {
        auto new_pc = And(value, Imm32(0xFFFFFFFE));
        Inst(IR::Opcode::SetRegister, { RegRef(Reg::PC), new_pc });
    }
}

void IREmitter::LoadWritePC(IR::ValuePtr value) {
    // This behaviour is ARM version-dependent.
    // The below implementation is for ARMv6k
    Inst(IR::Opcode::BXWritePC, {value});
}

void IREmitter::CallSupervisor(IR::ValuePtr value) {
    Inst(IR::Opcode::CallSupervisor, {value});
}

IR::ValuePtr IREmitter::GetCFlag() {
    return Inst(IR::Opcode::GetCFlag, {});
}

void IREmitter::SetNFlag(IR::ValuePtr value) {
    Inst(IR::Opcode::SetNFlag, {value});
}

void IREmitter::SetZFlag(IR::ValuePtr value) {
    Inst(IR::Opcode::SetZFlag, {value});
}

void IREmitter::SetCFlag(IR::ValuePtr value) {
    Inst(IR::Opcode::SetCFlag, {value});
}

void IREmitter::SetVFlag(IR::ValuePtr value) {
    Inst(IR::Opcode::SetVFlag, {value});
}

IR::ValuePtr IREmitter::LeastSignificantHalf(IR::ValuePtr value) {
    return Inst(IR::Opcode::LeastSignificantHalf, {value});
}

IR::ValuePtr IREmitter::LeastSignificantByte(IR::ValuePtr value) {
    return Inst(IR::Opcode::LeastSignificantByte, {value});
}

IR::ValuePtr IREmitter::MostSignificantBit(IR::ValuePtr value) {
    return Inst(IR::Opcode::MostSignificantBit, {value});
}

IR::ValuePtr IREmitter::IsZero(IR::ValuePtr value) {
    return Inst(IR::Opcode::IsZero, {value});
}

IREmitter::ResultAndCarry IREmitter::LogicalShiftLeft(IR::ValuePtr value_in, IR::ValuePtr shift_amount, IR::ValuePtr carry_in) {
    auto result = Inst(IR::Opcode::LogicalShiftLeft, {value_in, shift_amount, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IREmitter::ResultAndCarry IREmitter::LogicalShiftRight(IR::ValuePtr value_in, IR::ValuePtr shift_amount, IR::ValuePtr carry_in) {
    auto result = Inst(IR::Opcode::LogicalShiftRight, {value_in, shift_amount, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IREmitter::ResultAndCarry IREmitter::ArithmeticShiftRight(IR::ValuePtr value_in, IR::ValuePtr shift_amount, IR::ValuePtr carry_in) {
    auto result = Inst(IR::Opcode::ArithmeticShiftRight, {value_in, shift_amount, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IREmitter::ResultAndCarry IREmitter::RotateRight(IR::ValuePtr value_in, IR::ValuePtr shift_amount, IR::ValuePtr carry_in) {
    auto result = Inst(IR::Opcode::RotateRight, {value_in, shift_amount, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    return {result, carry_out};
}

IREmitter::ResultAndCarryAndOverflow IREmitter::AddWithCarry(IR::ValuePtr a, IR::ValuePtr b, IR::ValuePtr carry_in) {
    auto result = Inst(IR::Opcode::AddWithCarry, {a, b, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    auto overflow = Inst(IR::Opcode::GetOverflowFromOp, {result});
    return {result, carry_out, overflow};
}

IR::ValuePtr IREmitter::Add(IR::ValuePtr a, IR::ValuePtr b) {
    return Inst(IR::Opcode::AddWithCarry, {a, b, Imm1(0)});
}

IREmitter::ResultAndCarryAndOverflow IREmitter::SubWithCarry(IR::ValuePtr a, IR::ValuePtr b, IR::ValuePtr carry_in) {
    // This is equivalent to AddWithCarry(a, Not(b), carry_in).
    auto result = Inst(IR::Opcode::SubWithCarry, {a, b, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    auto overflow = Inst(IR::Opcode::GetOverflowFromOp, {result});
    return {result, carry_out, overflow};
}

IR::ValuePtr IREmitter::And(IR::ValuePtr a, IR::ValuePtr b) {
    return Inst(IR::Opcode::And, {a, b});
}

IR::ValuePtr IREmitter::Eor(IR::ValuePtr a, IR::ValuePtr b) {
    return Inst(IR::Opcode::Eor, {a, b});
}

IR::ValuePtr IREmitter::Or(IR::ValuePtr a, IR::ValuePtr b) {
    return Inst(IR::Opcode::Or, {a, b});
}

IR::ValuePtr IREmitter::Not(IR::ValuePtr a) {
    return Inst(IR::Opcode::Not, {a});
}

IR::ValuePtr IREmitter::ReadMemory8(IR::ValuePtr vaddr) {
    return Inst(IR::Opcode::ReadMemory8, {vaddr});
}

IR::ValuePtr IREmitter::ReadMemory16(IR::ValuePtr vaddr) {
    return Inst(IR::Opcode::ReadMemory16, {vaddr});
}

IR::ValuePtr IREmitter::ReadMemory32(IR::ValuePtr vaddr) {
    return Inst(IR::Opcode::ReadMemory32, {vaddr});
}

IR::ValuePtr IREmitter::ReadMemory64(IR::ValuePtr vaddr) {
    return Inst(IR::Opcode::ReadMemory64, {vaddr});
}

void IREmitter::WriteMemory8(IR::ValuePtr vaddr, IR::ValuePtr value) {
    Inst(IR::Opcode::WriteMemory8, {vaddr, value});
}

void IREmitter::WriteMemory16(IR::ValuePtr vaddr, IR::ValuePtr value) {
    Inst(IR::Opcode::WriteMemory16, {vaddr, value});
}

void IREmitter::WriteMemory32(IR::ValuePtr vaddr, IR::ValuePtr value) {
    Inst(IR::Opcode::WriteMemory32, {vaddr, value});
}

void IREmitter::WriteMemory64(IR::ValuePtr vaddr, IR::ValuePtr value) {
    Inst(IR::Opcode::WriteMemory64, {vaddr, value});
}


void IREmitter::SetTerm(const IR::Terminal& terminal) {
    ASSERT_MSG(block.terminal.which() == 0, "Terminal has already been set.");
    block.terminal = terminal;
}

IR::ValuePtr IREmitter::Inst(IR::Opcode op, std::initializer_list<IR::ValuePtr> args) {
    auto inst = std::make_shared<IR::Inst>(op);
    assert(args.size() == inst->NumArgs());

    std::for_each(args.begin(), args.end(), [&inst, op, index = size_t(0)](const auto& v) mutable {
        assert(IR::GetArgTypeOf(op, index) == v->GetType());
        inst->SetArg(index, v);
        index++;
    });

    AddToBlock(inst);
    return inst;
}

IR::ValuePtr IREmitter::RegRef(Reg reg) {
    auto regref = std::make_shared<IR::ImmRegRef>(reg);
    AddToBlock(regref);
    return regref;
}

void IREmitter::AddToBlock(IR::ValuePtr value) {
    block.instructions.emplace_back(value);
}

} // namespace Arm
} // namespace Dynarmic

