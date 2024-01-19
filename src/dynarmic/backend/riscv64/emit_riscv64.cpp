/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/riscv64/emit_riscv64.h"

#include <bit>

#include <biscuit/assembler.hpp>
#include <fmt/ostream.h>
#include <mcl/bit/bit_field.hpp>

#include "dynarmic/backend/riscv64/a32_jitstate.h"
#include "dynarmic/backend/riscv64/abi.h"
#include "dynarmic/backend/riscv64/emit_context.h"
#include "dynarmic/backend/riscv64/reg_alloc.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::RV64 {

template<IR::Opcode op>
void EmitIR(biscuit::Assembler&, EmitContext&, IR::Inst*) {
    ASSERT_FALSE("Unimplemented opcode {}", op);
}

template<>
void EmitIR<IR::Opcode::Void>(biscuit::Assembler&, EmitContext&, IR::Inst*) {}

template<>
void EmitIR<IR::Opcode::A32GetRegister>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::A32SetRegister>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::A32SetCpsrNZC>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::LogicalShiftLeft32>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst);

template<>
void EmitIR<IR::Opcode::GetCarryFromOp>(biscuit::Assembler&, EmitContext& ctx, IR::Inst* inst) {
    [[maybe_unused]] auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(ctx.reg_alloc.IsValueLive(inst));
}

template<>
void EmitIR<IR::Opcode::GetNZFromOp>(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Xvalue = ctx.reg_alloc.ReadX(args[0]);
    auto Xnz = ctx.reg_alloc.WriteX(inst);
    RegAlloc::Realize(Xvalue, Xnz);

    as.SEQZ(Xnz, Xvalue);
    as.SLLI(Xnz, Xnz, 30);
    as.SLT(Xscratch0, Xvalue, biscuit::zero);
    as.SLLI(Xscratch0, Xscratch0, 31);
    as.OR(Xnz, Xnz, Xscratch0);
}

EmittedBlockInfo EmitRV64(biscuit::Assembler& as, IR::Block block, const EmitConfig& emit_conf) {
    using namespace biscuit;

    EmittedBlockInfo ebi;

    RegAlloc reg_alloc{as, GPR_ORDER, FPR_ORDER};
    EmitContext ctx{block, reg_alloc, emit_conf, ebi};

    ebi.entry_point = reinterpret_cast<CodePtr>(as.GetCursorPointer());

    for (auto iter = block.begin(); iter != block.end(); ++iter) {
        IR::Inst* inst = &*iter;

        switch (inst->GetOpcode()) {
#define OPCODE(name, type, ...)                  \
    case IR::Opcode::name:                       \
        EmitIR<IR::Opcode::name>(as, ctx, inst); \
        break;
#define A32OPC(name, type, ...)                       \
    case IR::Opcode::A32##name:                       \
        EmitIR<IR::Opcode::A32##name>(as, ctx, inst); \
        break;
#define A64OPC(name, type, ...)                       \
    case IR::Opcode::A64##name:                       \
        EmitIR<IR::Opcode::A64##name>(as, ctx, inst); \
        break;
#include "dynarmic/ir/opcodes.inc"
#undef OPCODE
#undef A32OPC
#undef A64OPC
        default:
            ASSERT_FALSE("Invalid opcode: {}", inst->GetOpcode());
            break;
        }
    }

    reg_alloc.AssertNoMoreUses();

    // TODO: Add Cycles

    // TODO: Emit Terminal
    const auto term = block.GetTerminal();
    const IR::Term::LinkBlock* link_block_term = boost::get<IR::Term::LinkBlock>(&term);
    ASSERT(link_block_term);
    as.LI(Xscratch0, link_block_term->next.Value());
    as.SD(Xscratch0, offsetof(A32JitState, regs) + sizeof(u32) * 15, Xstate);

    ptrdiff_t offset = reinterpret_cast<CodePtr>(as.GetCursorPointer()) - ebi.entry_point;
    ebi.relocations.emplace_back(Relocation{offset, LinkTarget::ReturnFromRunCode});
    as.NOP();

    ebi.size = reinterpret_cast<CodePtr>(as.GetCursorPointer()) - ebi.entry_point;
    return ebi;
}

}  // namespace Dynarmic::Backend::RV64
