/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <fmt/ostream.h>
#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/a32_jitstate.h"
#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/backend/arm64/emit_context.h"
#include "dynarmic/backend/arm64/reg_alloc.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

template<>
void EmitIR<IR::Opcode::LogicalShiftLeft32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    // TODO: Use host flags
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];
    auto& carry_arg = args[2];

    if (!carry_inst) {
        if (shift_arg.IsImmediate()) {
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            RegAlloc::Realize(Wresult, Woperand);

            const u8 shift = shift_arg.GetImmediateU8();

            if (shift <= 31) {
                code.LSL(Wresult, Woperand, shift);
            } else {
                code.MOV(Wresult, WZR);
            }
        } else {
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            auto Wshift = ctx.reg_alloc.ReadW(shift_arg);
            RegAlloc::Realize(Wresult, Woperand, Wshift);
            ctx.reg_alloc.SpillFlags();

            code.AND(Wscratch0, Wshift, 0xff);
            code.LSL(Wresult, Woperand, Wscratch0);
            code.CMP(Wscratch0, 32);
            code.CSEL(Wresult, Wresult, WZR, LT);
        }
    } else {
        if (shift_arg.IsImmediate()) {
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            auto Wcarry_in = ctx.reg_alloc.ReadW(carry_arg);
            RegAlloc::Realize(Wresult, Wcarry_out, Woperand, Wcarry_in);

            const u8 shift = shift_arg.GetImmediateU8();

            if (shift == 0) {
                code.MOV(*Wresult, Woperand);
                code.MOV(*Wcarry_out, Wcarry_in);
            } else if (shift < 32) {
                code.UBFX(Wcarry_out, Woperand, 32 - shift, 1);
                code.LSL(Wresult, Woperand, shift);
            } else if (shift > 32) {
                code.MOV(Wresult, WZR);
                code.MOV(Wcarry_out, WZR);
            } else {
                code.AND(Wcarry_out, Wresult, 1);
                code.MOV(Wresult, WZR);
            }
        } else {
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            auto Wshift = ctx.reg_alloc.ReadW(shift_arg);
            auto Wcarry_in = ctx.reg_alloc.ReadW(carry_arg);
            RegAlloc::Realize(Wresult, Wcarry_out, Woperand, Wshift, Wcarry_in);
            ctx.reg_alloc.SpillFlags();

            // TODO: Use RMIF

            oaknut::Label zero, end;

            code.ANDS(Wscratch1, Wshift, 0xff);
            code.B(EQ, zero);

            code.NEG(Wscratch0, Wshift);
            code.LSR(Wcarry_out, Woperand, Wscratch0);
            code.LSL(Wresult, Woperand, Wshift);
            code.AND(Wcarry_out, Wcarry_out, 1);
            code.CMP(Wscratch1, 32);
            code.CSEL(Wresult, Wresult, WZR, LT);
            code.CSEL(Wcarry_out, Wcarry_out, WZR, LE);
            code.B(end);

            code.l(zero);
            code.MOV(*Wresult, Woperand);
            code.MOV(*Wcarry_out, Wcarry_in);

            code.l(end);
        }
    }
}

template<>
void EmitIR<IR::Opcode::MostSignificantBit>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    // TODO: Use host flags
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    auto Woperand = ctx.reg_alloc.ReadW(args[0]);
    RegAlloc::Realize(Wresult, Woperand);

    code.LSR(Wresult, Woperand, 31);
}

template<>
void EmitIR<IR::Opcode::IsZero32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    // TODO: Use host flags
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    auto Woperand = ctx.reg_alloc.ReadW(args[0]);
    RegAlloc::Realize(Wresult, Woperand);

    code.CMP(Woperand, 0);
    code.CSET(Wresult, EQ);
}

}  // namespace Dynarmic::Backend::Arm64
