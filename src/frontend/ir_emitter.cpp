/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/ir_emitter.h"

namespace Dynarmic {
namespace Arm {

void IREmitter::Unimplemented() {

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
        u32 offset = current_location.TFlag ? 4 : 8;
        return Imm32(current_location.arm_pc + offset);
    }
    return Inst(IR::Opcode::GetRegister, { RegRef(reg) });
}

void IREmitter::SetRegister(const Reg reg, IR::ValuePtr value) {
    Inst(IR::Opcode::SetRegister, { RegRef(reg), value });
}

void IREmitter::ALUWritePC(IR::ValuePtr value) {
    // This behaviour is ARM version-dependent.
    ASSERT_MSG(false, "Unimplemented");
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

IREmitter::ResultAndCarryAndOverflow IREmitter::AddWithCarry(IR::ValuePtr a, IR::ValuePtr b, IR::ValuePtr carry_in) {
    auto result = Inst(IR::Opcode::AddWithCarry, {a, b, carry_in});
    auto carry_out = Inst(IR::Opcode::GetCarryFromOp, {result});
    auto overflow = Inst(IR::Opcode::GetOverflowFromOp, {result});
    return {result, carry_out, overflow};
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

