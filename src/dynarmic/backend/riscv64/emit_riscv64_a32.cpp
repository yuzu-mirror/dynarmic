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
void EmitIR<IR::Opcode::A32GetRegister>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst) {
    const A32::Reg reg = inst->GetArg(0).GetA32RegRef();

    auto Xresult = ctx.reg_alloc.WriteX(inst);
    RegAlloc::Realize(Xresult);

    as.LWU(Xresult, offsetof(A32JitState, regs) + sizeof(u32) * static_cast<size_t>(reg), Xstate);
}

template<>
void EmitIR<IR::Opcode::A32SetRegister>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst) {
    const A32::Reg reg = inst->GetArg(0).GetA32RegRef();

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Xvalue = ctx.reg_alloc.ReadX(args[1]);
    RegAlloc::Realize(Xvalue);

    // TODO: Detect if Gpr vs Fpr is more appropriate

    as.SW(Xvalue, offsetof(A32JitState, regs) + sizeof(u32) * static_cast<size_t>(reg), Xstate);
}

template<>
void EmitIR<IR::Opcode::A32SetCpsrNZC>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    // TODO: Add full implementation
    ASSERT(!args[0].IsImmediate() && !args[1].IsImmediate());

    auto Xnz = ctx.reg_alloc.ReadX(args[0]);
    auto Xc = ctx.reg_alloc.ReadX(args[1]);
    RegAlloc::Realize(Xnz, Xc);

    as.LWU(Xscratch0, offsetof(A32JitState, cpsr_nzcv), Xstate);
    as.LUI(Xscratch1, 0x10000);
    as.AND(Xscratch0, Xscratch0, Xscratch1);
    as.OR(Xscratch0, Xscratch0, Xnz);
    as.OR(Xscratch0, Xscratch0, Xc);
    as.SW(Xscratch0, offsetof(A32JitState, cpsr_nzcv), Xstate);
}

}  // namespace Dynarmic::Backend::RV64
