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
        IR::Value result;
        IR::Value carry;
    };

    struct ResultAndCarryAndOverflow {
        IR::Value result;
        IR::Value carry;
        IR::Value overflow;
    };

    void Unimplemented();
    u32 PC();
    u32 AlignPC(size_t alignment);

    IR::Value Imm1(bool value);
    IR::Value Imm8(u8 value);
    IR::Value Imm32(u32 value);

    IR::Value GetRegister(Reg source_reg);
    void SetRegister(const Reg dest_reg, const IR::Value& value);

    void ALUWritePC(const IR::Value& value);
    void BranchWritePC(const IR::Value& value);
    void BXWritePC(const IR::Value& value);
    void LoadWritePC(const IR::Value& value);
    void CallSupervisor(const IR::Value& value);

    IR::Value GetCFlag();
    void SetNFlag(const IR::Value& value);
    void SetZFlag(const IR::Value& value);
    void SetCFlag(const IR::Value& value);
    void SetVFlag(const IR::Value& value);

    IR::Value Pack2x32To1x64(const IR::Value& lo, const IR::Value& hi);
    IR::Value LeastSignificantWord(const IR::Value& value);
    IR::Value MostSignificantWord(const IR::Value& value);
    IR::Value LeastSignificantHalf(const IR::Value& value);
    IR::Value LeastSignificantByte(const IR::Value& value);
    IR::Value MostSignificantBit(const IR::Value& value);
    IR::Value IsZero(const IR::Value& value);
    IR::Value IsZero64(const IR::Value& value);

    ResultAndCarry LogicalShiftLeft(const IR::Value& value_in, const IR::Value& shift_amount, const IR::Value& carry_in);
    ResultAndCarry LogicalShiftRight(const IR::Value& value_in, const IR::Value& shift_amount, const IR::Value& carry_in);
    ResultAndCarry ArithmeticShiftRight(const IR::Value& value_in, const IR::Value& shift_amount, const IR::Value& carry_in);
    ResultAndCarry RotateRight(const IR::Value& value_in, const IR::Value& shift_amount, const IR::Value& carry_in);
    ResultAndCarry RotateRightExtended(const IR::Value& value_in, const IR::Value& carry_in);
    ResultAndCarryAndOverflow AddWithCarry(const IR::Value& a, const IR::Value& b, const IR::Value& carry_in);
    IR::Value Add(const IR::Value& a, const IR::Value& b);
    IR::Value Add64(const IR::Value& a, const IR::Value& b);
    ResultAndCarryAndOverflow SubWithCarry(const IR::Value& a, const IR::Value& b, const IR::Value& carry_in);
    IR::Value Sub(const IR::Value& a, const IR::Value& b);
    IR::Value Mul(const IR::Value& a, const IR::Value& b);
    IR::Value Mul64(const IR::Value& a, const IR::Value& b);
    IR::Value SignedMulHi(const IR::Value& a, const IR::Value& b);
    IR::Value UnsignedMulHi(const IR::Value& a, const IR::Value& b);
    IR::Value And(const IR::Value& a, const IR::Value& b);
    IR::Value Eor(const IR::Value& a, const IR::Value& b);
    IR::Value Or(const IR::Value& a, const IR::Value& b);
    IR::Value Not(const IR::Value& a);
    IR::Value SignExtendWordToLong(const IR::Value& a);
    IR::Value SignExtendHalfToWord(const IR::Value& a);
    IR::Value SignExtendByteToWord(const IR::Value& a);
    IR::Value ZeroExtendWordToLong(const IR::Value& a);
    IR::Value ZeroExtendHalfToWord(const IR::Value& a);
    IR::Value ZeroExtendByteToWord(const IR::Value& a);
    IR::Value ByteReverseWord(const IR::Value& a);
    IR::Value ByteReverseHalf(const IR::Value& a);
    IR::Value ByteReverseDual(const IR::Value& a);

    IR::Value ReadMemory8(const IR::Value& vaddr);
    IR::Value ReadMemory16(const IR::Value& vaddr);
    IR::Value ReadMemory32(const IR::Value& vaddr);
    IR::Value ReadMemory64(const IR::Value& vaddr);
    void WriteMemory8(const IR::Value& vaddr, const IR::Value& value);
    void WriteMemory16(const IR::Value& vaddr, const IR::Value& value);
    void WriteMemory32(const IR::Value& vaddr, const IR::Value& value);
    void WriteMemory64(const IR::Value& vaddr, const IR::Value& value);

    void SetTerm(const IR::Terminal& terminal);

private:
    IR::Value Inst(IR::Opcode op, std::initializer_list<IR::Value> args);
};

} // namespace Arm
} // namespace Dynarmic
