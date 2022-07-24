/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <fmt/ostream.h>
#include <mcl/bit/bit_field.hpp>
#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/a32_jitstate.h"
#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/backend/arm64/emit_context.h"
#include "dynarmic/backend/arm64/reg_alloc.h"
#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

oaknut::Label EmitA32Cond(oaknut::CodeGenerator& code, EmitContext&, IR::Cond cond) {
    oaknut::Label pass;
    // TODO: Flags in host flags
    code.LDR(Wscratch0, Xstate, offsetof(A32JitState, cpsr_nzcv));
    code.MSR(oaknut::SystemReg::NZCV, Xscratch0);
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
void EmitIR<IR::Opcode::A32SetCheckBit>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wbit = ctx.reg_alloc.ReadW(args[0]);
    RegAlloc::Realize(Wbit);

    code.STRB(Wbit, SP, offsetof(StackLayout, check_bit));
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
void EmitIR<IR::Opcode::A32GetExtendedRegister32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
    ASSERT(A32::IsSingleExtReg(reg));
    const size_t index = static_cast<size_t>(reg) - static_cast<size_t>(A32::ExtReg::S0);

    auto Sresult = ctx.reg_alloc.WriteS(inst);
    RegAlloc::Realize(Sresult);

    // TODO: Detect if Gpr vs Fpr is more appropriate

    code.LDR(Sresult, Xstate, offsetof(A32JitState, ext_regs) + sizeof(u32) * index);
}

template<>
void EmitIR<IR::Opcode::A32GetVector>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32GetExtendedRegister64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
    ASSERT(A32::IsDoubleExtReg(reg));
    const size_t index = static_cast<size_t>(reg) - static_cast<size_t>(A32::ExtReg::D0);

    auto Dresult = ctx.reg_alloc.WriteD(inst);
    RegAlloc::Realize(Dresult);

    // TODO: Detect if Gpr vs Fpr is more appropriate

    code.LDR(Dresult, Xstate, offsetof(A32JitState, ext_regs) + 2 * sizeof(u32) * index);
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
void EmitIR<IR::Opcode::A32SetExtendedRegister32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetExtendedRegister64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetVector>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32GetCpsr>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetCpsr>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetCpsrNZCV>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wnzcv = ctx.reg_alloc.ReadW(args[0]);
    RegAlloc::Realize(Wnzcv);

    code.STR(Wnzcv, Xstate, offsetof(A32JitState, cpsr_nzcv));
}

template<>
void EmitIR<IR::Opcode::A32SetCpsrNZCVRaw>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetCpsrNZCVQ>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetCpsrNZ>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wnz = ctx.reg_alloc.ReadW(args[0]);
    RegAlloc::Realize(Wnz);

    // TODO: Track latent value

    code.LDR(Wscratch0, Xstate, offsetof(A32JitState, cpsr_nzcv));
    code.AND(Wscratch0, Wscratch0, 0x30000000);
    code.ORR(Wscratch0, Wscratch0, Wnz);
    code.STR(Wscratch0, Xstate, offsetof(A32JitState, cpsr_nzcv));
}

template<>
void EmitIR<IR::Opcode::A32SetCpsrNZC>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wnz = ctx.reg_alloc.ReadW(args[0]);
    auto Wc = ctx.reg_alloc.ReadW(args[1]);
    RegAlloc::Realize(Wnz, Wc);

    // TODO: Track latent value

    code.LDR(Wscratch0, Xstate, offsetof(A32JitState, cpsr_nzcv));
    code.AND(Wscratch0, Wscratch0, 0x10000000);
    code.ORR(Wscratch0, Wscratch0, Wnz);
    code.ORR(Wscratch0, Wscratch0, Wc);
    code.STR(Wscratch0, Xstate, offsetof(A32JitState, cpsr_nzcv));
}

template<>
void EmitIR<IR::Opcode::A32GetCFlag>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto Wflag = ctx.reg_alloc.WriteW(inst);
    RegAlloc::Realize(Wflag);

    code.LDR(Wflag, Xstate, offsetof(A32JitState, cpsr_nzcv));
    code.AND(Wflag, Wflag, 1 << 29);
}

template<>
void EmitIR<IR::Opcode::A32OrQFlag>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32GetGEFlags>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetGEFlags>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetGEFlagsCompressed>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32BXWritePC>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const u32 upper_without_t = (A32::LocationDescriptor{ctx.block.EndLocation()}.SetSingleStepping(false).UniqueHash() >> 32) & 0xFFFFFFFE;

    static_assert(offsetof(A32JitState, regs) + 16 * sizeof(u32) == offsetof(A32JitState, upper_location_descriptor));

    if (args[0].IsImmediate()) {
        const u32 new_pc = args[0].GetImmediateU32();
        const u32 mask = mcl::bit::get_bit<0>(new_pc) ? 0xFFFFFFFE : 0xFFFFFFFC;
        const u32 new_upper = upper_without_t | (mcl::bit::get_bit<0>(new_pc) ? 1 : 0);

        code.MOV(Xscratch0, (u64{new_upper} << 32) | (new_pc & mask));
        code.STUR(Xscratch0, Xstate, offsetof(A32JitState, regs) + 15 * sizeof(u32));
    } else {
        auto Wpc = ctx.reg_alloc.ReadW(args[0]);
        RegAlloc::Realize(Wpc);
        ctx.reg_alloc.SpillFlags();

        code.ANDS(Wscratch0, Wpc, 1);
        code.MOV(Wscratch1, 3);
        code.CSEL(Wscratch1, Wscratch0, Wscratch1, NE);
        code.BIC(Wscratch1, Wpc, Wscratch1);
        code.MOV(Wscratch0, upper_without_t);
        code.CINC(Wscratch0, Wscratch0, NE);
        code.STP(Wscratch1, Wscratch0, Xstate, offsetof(A32JitState, regs) + 15 * sizeof(u32));
    }
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

template<>
void EmitIR<IR::Opcode::A32CallSupervisor>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32ExceptionRaised>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32DataSynchronizationBarrier>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32DataMemoryBarrier>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32InstructionSynchronizationBarrier>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32GetFpscr>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetFpscr>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32GetFpscrNZCV>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::A32SetFpscrNZCV>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

}  // namespace Dynarmic::Backend::Arm64
