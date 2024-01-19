/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <biscuit/assembler.hpp>
#include <fmt/ostream.h>

#include "dynarmic/backend/riscv64/a32_jitstate.h"
#include "dynarmic/backend/riscv64/abi.h"
#include "dynarmic/backend/riscv64/emit_context.h"
#include "dynarmic/backend/riscv64/emit_riscv64.h"
#include "dynarmic/backend/riscv64/reg_alloc.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::RV64 {

template<>
void EmitIR<IR::Opcode::LogicalShiftLeft32>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];
    auto& carry_arg = args[2];

    // TODO: Add full implementation
    ASSERT(carry_inst != nullptr);
    ASSERT(shift_arg.IsImmediate());

    auto Xresult = ctx.reg_alloc.WriteX(inst);
    auto Xcarry_out = ctx.reg_alloc.WriteX(carry_inst);
    auto Xoperand = ctx.reg_alloc.ReadX(operand_arg);
    auto Xcarry_in = ctx.reg_alloc.ReadX(carry_arg);
    RegAlloc::Realize(Xresult, Xcarry_out, Xoperand, Xcarry_in);

    const u8 shift = shift_arg.GetImmediateU8();

    if (shift == 0) {
        as.ADDW(Xresult, Xoperand, biscuit::zero);
        as.ADDW(Xcarry_out, Xcarry_in, biscuit::zero);
    } else if (shift < 32) {
        as.SRLIW(Xcarry_out, Xoperand, 32 - shift);
        as.ANDI(Xcarry_out, Xcarry_out, 1);
        as.SLLIW(Xresult, Xoperand, shift);
    } else if (shift > 32) {
        as.MV(Xresult, biscuit::zero);
        as.MV(Xcarry_out, biscuit::zero);
    } else {
        as.ANDI(Xcarry_out, Xresult, 1);
        as.MV(Xresult, biscuit::zero);
    }
}

}  // namespace Dynarmic::Backend::RV64
