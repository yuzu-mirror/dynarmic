/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "frontend/arm_types.h"
#include "frontend/ir/ir.h"
#include "frontend/ir/opcodes.h"

namespace Dynarmic {
namespace Arm {

class IREmitter {
public:
    explicit IREmitter(LocationDescriptor descriptor) : block(descriptor), current_location(descriptor) {}

    IR::Block block;
    LocationDescriptor current_location;

    struct ResultAndCarry {
        IR::ValuePtr result;
        IR::ValuePtr carry;
    };

    struct ResultAndCarryAndOverflow {
        IR::ValuePtr result;
        IR::ValuePtr carry;
        IR::ValuePtr overflow;
    };

    void Unimplemented();
    u32 PC();
    u32 AlignPC(size_t alignment);

    IR::ValuePtr Imm1(bool value);
    IR::ValuePtr Imm8(u8 value);
    IR::ValuePtr Imm32(u32 value);

    IR::ValuePtr GetRegister(Reg source_reg);
    void SetRegister(const Reg dest_reg, IR::ValuePtr value);

    void ALUWritePC(IR::ValuePtr value);
    void BranchWritePC(IR::ValuePtr value);
    void BXWritePC(IR::ValuePtr value);
    void LoadWritePC(IR::ValuePtr value);
    void CallSupervisor(IR::ValuePtr value);

    IR::ValuePtr GetCFlag();
    void SetNFlag(IR::ValuePtr value);
    void SetZFlag(IR::ValuePtr value);
    void SetCFlag(IR::ValuePtr value);
    void SetVFlag(IR::ValuePtr value);

    IR::ValuePtr LeastSignificantHalf(IR::ValuePtr value);
    IR::ValuePtr LeastSignificantByte(IR::ValuePtr value);
    IR::ValuePtr MostSignificantBit(IR::ValuePtr value);
    IR::ValuePtr IsZero(IR::ValuePtr value);

    ResultAndCarry LogicalShiftLeft(IR::ValuePtr value_in, IR::ValuePtr shift_amount, IR::ValuePtr carry_in);
    ResultAndCarry LogicalShiftRight(IR::ValuePtr value_in, IR::ValuePtr shift_amount, IR::ValuePtr carry_in);
    ResultAndCarry ArithmeticShiftRight(IR::ValuePtr value_in, IR::ValuePtr shift_amount, IR::ValuePtr carry_in);
    ResultAndCarry RotateRight(IR::ValuePtr value_in, IR::ValuePtr shift_amount, IR::ValuePtr carry_in);
    ResultAndCarryAndOverflow AddWithCarry(IR::ValuePtr a, IR::ValuePtr b, IR::ValuePtr carry_in);
    IR::ValuePtr Add(IR::ValuePtr a, IR::ValuePtr b);
    ResultAndCarryAndOverflow SubWithCarry(IR::ValuePtr a, IR::ValuePtr b, IR::ValuePtr carry_in);
    IR::ValuePtr Sub(IR::ValuePtr a, IR::ValuePtr b);
    IR::ValuePtr And(IR::ValuePtr a, IR::ValuePtr b);
    IR::ValuePtr Eor(IR::ValuePtr a, IR::ValuePtr b);
    IR::ValuePtr Or(IR::ValuePtr a, IR::ValuePtr b);
    IR::ValuePtr Not(IR::ValuePtr a);
    IR::ValuePtr SignExtendHalfToWord(IR::ValuePtr a);
    IR::ValuePtr SignExtendByteToWord(IR::ValuePtr a);
    IR::ValuePtr ZeroExtendHalfToWord(IR::ValuePtr a);
    IR::ValuePtr ZeroExtendByteToWord(IR::ValuePtr a);
    IR::ValuePtr ByteReverseWord(IR::ValuePtr a);
    IR::ValuePtr ByteReverseHalf(IR::ValuePtr a);

    IR::ValuePtr ReadMemory8(IR::ValuePtr vaddr);
    IR::ValuePtr ReadMemory16(IR::ValuePtr vaddr);
    IR::ValuePtr ReadMemory32(IR::ValuePtr vaddr);
    IR::ValuePtr ReadMemory64(IR::ValuePtr vaddr);
    void WriteMemory8(IR::ValuePtr vaddr, IR::ValuePtr value);
    void WriteMemory16(IR::ValuePtr vaddr, IR::ValuePtr value);
    void WriteMemory32(IR::ValuePtr vaddr, IR::ValuePtr value);
    void WriteMemory64(IR::ValuePtr vaddr, IR::ValuePtr value);

    void SetTerm(const IR::Terminal& terminal);

private:
    IR::ValuePtr Inst(IR::Opcode op, std::initializer_list<IR::ValuePtr> args);
    IR::ValuePtr RegRef(Reg reg);
    void AddToBlock(IR::ValuePtr value);
};

} // namespace Arm
} // namespace Dynarmic
