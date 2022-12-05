/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/arm64/emit_arm64_memory.h"

#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/backend/arm64/emit_context.h"
#include "dynarmic/backend/arm64/reg_alloc.h"
#include "dynarmic/ir/acc_type.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

bool IsOrdered(IR::AccType acctype) {
    return acctype == IR::AccType::ORDERED || acctype == IR::AccType::ORDEREDRW || acctype == IR::AccType::LIMITEDORDERED;
}

void EmitReadMemory(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.PrepareForCall({}, args[1]);
    const bool ordered = IsOrdered(args[2].GetImmediateAccType());

    EmitRelocation(code, ctx, fn);
    if (ordered) {
        code.DMB(oaknut::BarrierOp::ISH);
    }
    ctx.reg_alloc.DefineAsRegister(inst, X0);
}

void EmitReadMemory128(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.PrepareForCall({}, args[1]);
    const bool ordered = IsOrdered(args[2].GetImmediateAccType());

    EmitRelocation(code, ctx, fn);
    if (ordered) {
        code.DMB(oaknut::BarrierOp::ISH);
    }
    code.MOV(Q8.B16(), Q0.B16());
    ctx.reg_alloc.DefineAsRegister(inst, Q8);
}

void EmitExclusiveReadMemory(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.PrepareForCall({}, args[1]);
    const bool ordered = IsOrdered(args[2].GetImmediateAccType());

    code.MOV(Wscratch0, 1);
    code.STRB(Wscratch0, Xstate, ctx.conf.state_exclusive_state_offset);
    EmitRelocation(code, ctx, fn);
    if (ordered) {
        code.DMB(oaknut::BarrierOp::ISH);
    }
    ctx.reg_alloc.DefineAsRegister(inst, X0);
}

void EmitExclusiveReadMemory128(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.PrepareForCall({}, args[1]);
    const bool ordered = IsOrdered(args[2].GetImmediateAccType());

    code.MOV(Wscratch0, 1);
    code.STRB(Wscratch0, Xstate, ctx.conf.state_exclusive_state_offset);
    EmitRelocation(code, ctx, fn);
    if (ordered) {
        code.DMB(oaknut::BarrierOp::ISH);
    }
    code.MOV(Q8.B16(), Q0.B16());
    ctx.reg_alloc.DefineAsRegister(inst, Q8);
}

void EmitWriteMemory(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.PrepareForCall({}, args[1], args[2]);
    const bool ordered = IsOrdered(args[3].GetImmediateAccType());

    if (ordered) {
        code.DMB(oaknut::BarrierOp::ISH);
    }
    EmitRelocation(code, ctx, fn);
    if (ordered) {
        code.DMB(oaknut::BarrierOp::ISH);
    }
}

void EmitExclusiveWriteMemory(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.PrepareForCall({}, args[1], args[2]);
    const bool ordered = IsOrdered(args[3].GetImmediateAccType());

    oaknut::Label end;

    if (ordered) {
        code.DMB(oaknut::BarrierOp::ISH);
    }
    code.MOV(W0, 1);
    code.LDRB(Wscratch0, Xstate, ctx.conf.state_exclusive_state_offset);
    code.CBZ(Wscratch0, end);
    code.STRB(WZR, Xstate, ctx.conf.state_exclusive_state_offset);
    EmitRelocation(code, ctx, fn);
    if (ordered) {
        code.DMB(oaknut::BarrierOp::ISH);
    }
    code.l(end);
    ctx.reg_alloc.DefineAsRegister(inst, X0);
}

}  // namespace Dynarmic::Backend::Arm64
