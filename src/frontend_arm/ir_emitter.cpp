/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "ir_emitter.h"

namespace Dynarmic {
namespace Arm {

void IREmitter::Unimplemented() {

}

IR::ValuePtr IREmitter::Imm8(u8 i) {
    auto imm8 = std::make_shared<IR::ImmU8>(i);
    AddToBlock(imm8);
    return imm8;
}

IR::ValuePtr IREmitter::GetRegister(Dynarmic::Arm::Reg reg) {
    return Inst(IR::Opcode::GetRegister, { RegRef(reg) });
}

void IREmitter::SetRegister(const Reg reg, IR::ValuePtr value) {
    Inst(IR::Opcode::SetRegister, { RegRef(reg), value });
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

