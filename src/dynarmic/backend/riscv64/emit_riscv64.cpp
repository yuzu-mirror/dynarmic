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

// TODO: We should really move this to biscuit.
static void Mov64(biscuit::Assembler& as, biscuit::GPR rd, u64 imm) {
    if (mcl::bit::sign_extend<32>(imm) == imm) {
        as.LI(rd, static_cast<u32>(imm));
        return;
    }

    // For 64-bit imm, a sequence of up to 8 instructions (i.e. LUI+ADDIW+SLLI+ADDI+SLLI+ADDI+SLLI+ADDI) is emitted.
    // In the following, imm is processed from LSB to MSB while instruction emission is performed from MSB to LSB by calling Mov64 recursively.
    // In each recursion, the lowest 12 bits are removed from imm and the optimal shift amount is calculated.
    // Then, the remaining part of imm is processed recursively and as.LI() get called as soon as it fits into 32 bits.
    s32 lo12 = static_cast<s32>(mcl::bit::sign_extend<12>(imm));
    /* Add 0x800 to cancel out the signed extension of ADDI. */
    u64 hi52 = (imm + 0x800) >> 12;
    int shift = 12 + std::countr_zero(hi52);
    hi52 = mcl::bit::sign_extend(shift, hi52 >> (shift - 12));
    Mov64(as, rd, hi52);
    as.SLLI(rd, rd, shift);
    if (lo12 != 0) {
        as.ADDI(rd, rd, lo12);
    }
}

template<IR::Opcode op>
void EmitIR(biscuit::Assembler&, EmitContext&, IR::Inst*) {
    ASSERT_FALSE("Unimplemented opcode {}", op);
}

template<>
void EmitIR<IR::Opcode::GetCarryFromOp>(biscuit::Assembler&, EmitContext& ctx, IR::Inst* inst) {
    ASSERT(ctx.reg_alloc.IsValueLive(inst));
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
    // TODO: Add Cycles

    // TODO: Emit Terminal
    const auto term = block.GetTerminal();
    const IR::Term::LinkBlock* link_block_term = boost::get<IR::Term::LinkBlock>(&term);
    ASSERT(link_block_term);
    Mov64(as, Xscratch0, link_block_term->next.Value());
    as.SD(Xscratch0, offsetof(A32JitState, regs) + sizeof(u32) * 15, Xstate);

    ptrdiff_t offset = reinterpret_cast<CodePtr>(as.GetCursorPointer()) - ebi.entry_point;
    ebi.relocations.emplace_back(Relocation{offset, LinkTarget::ReturnFromRunCode});
    as.NOP();

    ebi.size = reinterpret_cast<CodePtr>(as.GetCursorPointer()) - ebi.entry_point;
    return ebi;
}

}  // namespace Dynarmic::Backend::RV64
