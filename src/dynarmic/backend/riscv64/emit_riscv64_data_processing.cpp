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

template<size_t bitsize>
static void AddImmWithFlags(biscuit::Assembler& as, biscuit::GPR rd, biscuit::GPR rs, u64 imm, biscuit::GPR flags) {
    static_assert(bitsize == 32 || bitsize == 64);
    if constexpr (bitsize == 32) {
        imm = static_cast<u32>(imm);
    }
    if (mcl::bit::sign_extend<12>(imm) == imm) {
        as.ADDIW(rd, rs, imm);
    } else {
        as.LI(Xscratch0, imm);
        as.ADDW(rd, rs, Xscratch0);
    }

    // N
    as.SEQZ(flags, rd);
    as.SLLI(flags, flags, 30);

    // Z
    as.SLTZ(Xscratch1, rd);
    as.SLLI(Xscratch1, Xscratch1, 31);
    as.OR(flags, flags, Xscratch1);

    // C
    if (mcl::bit::sign_extend<12>(imm) == imm) {
        as.ADDI(Xscratch1, rs, imm);
    } else {
        as.ADD(Xscratch1, rs, Xscratch0);
    }
    as.SRLI(Xscratch1, Xscratch1, 3);
    as.LUI(Xscratch0, 0x20000);
    as.AND(Xscratch1, Xscratch1, Xscratch0);
    as.OR(flags, flags, Xscratch1);

    // V
    as.LI(Xscratch0, imm);
    as.ADD(Xscratch1, rs, Xscratch0);
    as.XOR(Xscratch0, Xscratch0, rs);
    as.NOT(Xscratch0, Xscratch0);
    as.XOR(Xscratch1, Xscratch1, rs);
    as.AND(Xscratch1, Xscratch0, Xscratch1);
    as.SRLIW(Xscratch1, Xscratch1, 31);
    as.SLLI(Xscratch1, Xscratch1, 28);
    as.OR(flags, flags, Xscratch1);
}

template<size_t bitsize>
static void EmitSub(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst) {
    const auto nzcv_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetNZCVFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Xresult = ctx.reg_alloc.WriteX(inst);
    auto Xa = ctx.reg_alloc.ReadX(args[0]);

    if (nzcv_inst) {
        if (args[1].IsImmediate()) {
            const u64 imm = args[1].GetImmediateU64();

            if (args[2].IsImmediate()) {
                auto Xflags = ctx.reg_alloc.WriteX(nzcv_inst);
                RegAlloc::Realize(Xresult, Xflags, Xa);

                if (args[2].GetImmediateU1()) {
                    AddImmWithFlags<bitsize>(as, *Xresult, *Xa, ~imm, *Xflags);
                } else {
                    AddImmWithFlags<bitsize>(as, *Xresult, *Xa, -imm, *Xflags);
                }
            } else {
                ASSERT_FALSE("Unimplemented");
            }
        } else {
            ASSERT_FALSE("Unimplemented");
        }
    } else {
        ASSERT_FALSE("Unimplemented");
    }
}

template<>
void EmitIR<IR::Opcode::Sub32>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst) {
    EmitSub<32>(as, ctx, inst);
}

}  // namespace Dynarmic::Backend::RV64
