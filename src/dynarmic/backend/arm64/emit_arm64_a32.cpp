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

oaknut::Label EmitA32Cond(oaknut::CodeGenerator& code, EmitContext&, IR::Cond cond) {
    oaknut::Label pass;
    // TODO: Flags in host flags
    code.LDR(Wscratch0, Xstate, offsetof(A32JitState, cpsr_nzcv));
    code.MSR(static_cast<oaknut::SystemReg>(0b11'011'0100'0010'000), Xscratch0);
    code.B(static_cast<oaknut::Cond>(cond), pass);
    return pass;
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::Terminal terminal, IR::LocationDescriptor initial_location, bool is_single_step);

void EmitA32Terminal(oaknut::CodeGenerator&, EmitContext&, IR::Term::Interpret, IR::LocationDescriptor, bool) {
    ASSERT_FALSE("Interpret should never be emitted.");
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::ReturnToDispatch, IR::LocationDescriptor, bool) {
    EmitRelocation(code, ctx, LinkTarget::ReturnFromRunCode);
}

void EmitSetUpperLocationDescriptor(oaknut::CodeGenerator& code, EmitContext& ctx, IR::LocationDescriptor new_location, IR::LocationDescriptor old_location) {
    auto get_upper = [](const IR::LocationDescriptor& desc) -> u32 {
        return static_cast<u32>(A32::LocationDescriptor{desc}.SetSingleStepping(false).UniqueHash() >> 32);
    };

    const u32 old_upper = get_upper(old_location);
    const u32 new_upper = [&] {
        const u32 mask = ~u32(ctx.emit_conf.always_little_endian ? 0x2 : 0);
        return get_upper(new_location) & mask;
    }();

    if (old_upper != new_upper) {
        code.MOV(Wscratch0, new_upper);
        code.STR(Wscratch0, Xstate, offsetof(A32JitState, upper_location_descriptor));
    }
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::LinkBlock terminal, IR::LocationDescriptor initial_location, bool) {
    EmitSetUpperLocationDescriptor(code, ctx, terminal.next, initial_location);

    code.MOV(Xscratch0, terminal.next.Value());
    code.STUR(Xscratch0, Xstate, offsetof(A32JitState, regs) + sizeof(u32) * 15);
    EmitRelocation(code, ctx, LinkTarget::ReturnFromRunCode);

    // TODO: Implement LinkBlock optimization
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::LinkBlockFast terminal, IR::LocationDescriptor initial_location, bool) {
    EmitSetUpperLocationDescriptor(code, ctx, terminal.next, initial_location);

    code.MOV(Xscratch0, terminal.next.Value());
    code.STUR(Xscratch0, Xstate, offsetof(A32JitState, regs) + sizeof(u32) * 15);
    EmitRelocation(code, ctx, LinkTarget::ReturnFromRunCode);

    // TODO: Implement LinkBlockFast optimization
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::PopRSBHint, IR::LocationDescriptor, bool) {
    EmitRelocation(code, ctx, LinkTarget::ReturnFromRunCode);

    // TODO: Implement PopRSBHint optimization
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::FastDispatchHint, IR::LocationDescriptor, bool) {
    EmitRelocation(code, ctx, LinkTarget::ReturnFromRunCode);

    // TODO: Implement FastDispatchHint optimization
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::If terminal, IR::LocationDescriptor initial_location, bool is_single_step) {
    oaknut::Label pass = EmitA32Cond(code, ctx, terminal.if_);
    EmitA32Terminal(code, ctx, terminal.else_, initial_location, is_single_step);
    code.l(pass);
    EmitA32Terminal(code, ctx, terminal.then_, initial_location, is_single_step);
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::CheckBit terminal, IR::LocationDescriptor initial_location, bool is_single_step) {
    oaknut::Label fail;
    code.LDRB(Wscratch0, Xstate, offsetof(StackLayout, check_bit));
    code.CBZ(Wscratch0, fail);
    EmitA32Terminal(code, ctx, terminal.then_, initial_location, is_single_step);
    code.l(fail);
    EmitA32Terminal(code, ctx, terminal.else_, initial_location, is_single_step);
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::CheckHalt terminal, IR::LocationDescriptor initial_location, bool is_single_step) {
    oaknut::Label fail;
    code.LDAR(Wscratch0, Xhalt);
    code.CBNZ(Wscratch0, fail);
    EmitA32Terminal(code, ctx, terminal.else_, initial_location, is_single_step);
    code.l(fail);
    EmitRelocation(code, ctx, LinkTarget::ReturnFromRunCode);
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Term::Terminal terminal, IR::LocationDescriptor initial_location, bool is_single_step) {
    boost::apply_visitor([&](const auto& t) { EmitA32Terminal(code, ctx, t, initial_location, is_single_step); }, terminal);
}

void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx) {
    const A32::LocationDescriptor location{ctx.block.Location()};
    EmitA32Terminal(code, ctx, ctx.block.GetTerminal(), location.SetSingleStepping(false), location.SingleStepping());
}

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

template<>
void EmitIR<IR::Opcode::A32UpdateUpperLocationDescriptor>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst*) {
    for (auto& inst : ctx.block) {
        if (inst.GetOpcode() == IR::Opcode::A32BXWritePC) {
            return;
        }
    }
    EmitSetUpperLocationDescriptor(code, ctx, ctx.block.EndLocation(), ctx.block.Location());
}

}  // namespace Dynarmic::Backend::Arm64
