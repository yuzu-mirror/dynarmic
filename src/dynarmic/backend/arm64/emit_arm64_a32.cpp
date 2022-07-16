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
void EmitIR<IR::Opcode::A32GetRegister>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const A32::Reg reg = inst->GetArg(0).GetA32RegRef();

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    RegAlloc::Realize(Wresult);

    // TODO: Detect if Gpr vs Fpr is more appropriate

    code.LDR(Wresult, Xstate, offsetof(A32JitState, regs) + sizeof(u32) * static_cast<size_t>(reg));
}

template<>
void EmitIR<IR::Opcode::A32SetRegister>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const A32::Reg reg = inst->GetArg(0).GetA32RegRef();

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wvalue = ctx.reg_alloc.ReadW(args[1]);
    RegAlloc::Realize(Wvalue);

    // TODO: Detect if Gpr vs Fpr is more appropriate

    code.STR(Wvalue, Xstate, offsetof(A32JitState, regs) + sizeof(u32) * static_cast<size_t>(reg));
}

template<>
void EmitIR<IR::Opcode::A32GetCFlag>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto Wflag = ctx.reg_alloc.WriteW(inst);
    RegAlloc::Realize(Wflag);

    // TODO: Store in Flags

    code.LDR(Wscratch0, Xstate, offsetof(A32JitState, cpsr_nzcv));
    code.LSR(Wflag, Wscratch0, 29);
    code.AND(Wflag, Wflag, 1);
}

template<>
void EmitIR<IR::Opcode::A32SetCpsrNZC>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wnz = ctx.reg_alloc.ReadW(args[0]);
    auto Wc = ctx.reg_alloc.ReadW(args[1]);
    RegAlloc::Realize(Wnz, Wc);

    // TODO: Store in Flags
    code.LDR(Wscratch0, Xstate, offsetof(A32JitState, cpsr_nzcv));
    code.AND(Wscratch0, Wscratch0, 0x10000000);
    code.ORR(Wscratch0, Wscratch0, Wnz);
    code.SBFX(Wscratch0, Wc, 29, 1);
    code.STR(Wscratch0, Xstate, offsetof(A32JitState, cpsr_nzcv));
}

}  // namespace Dynarmic::Backend::Arm64
