/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/arm64/emit_arm64.h"

#include <fmt/ostream.h>
#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/a32_jitstate.h"
#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/emit_context.h"
#include "dynarmic/backend/arm64/reg_alloc.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

template<IR::Opcode op>
void EmitIR(oaknut::CodeGenerator&, EmitContext&, IR::Inst*) {
    ASSERT_FALSE("Unimplemented opcode {}", op);
}

template<>
void EmitIR<IR::Opcode::A32SetCheckBit>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::A32GetRegister>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::A32SetRegister>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::A32GetCFlag>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::A32SetCpsrNZC>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::A32BXWritePC>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::A32UpdateUpperLocationDescriptor>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst*);
template<>
void EmitIR<IR::Opcode::LogicalShiftLeft32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);
template<>
void EmitIR<IR::Opcode::MostSignificantBit>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);

template<>
void EmitIR<IR::Opcode::GetCarryFromOp>(oaknut::CodeGenerator&, EmitContext& ctx, IR::Inst* inst) {
    [[maybe_unused]] auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(ctx.reg_alloc.IsValueLive(inst));
}

template<>
void EmitIR<IR::Opcode::GetNZFromOp>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wvalue = ctx.reg_alloc.ReadW(args[0]);
    auto flags = ctx.reg_alloc.WriteFlags(inst);
    RegAlloc::Realize(Wvalue, flags);

    code.CMP(*Wvalue, WZR);
}

EmittedBlockInfo EmitArm64(oaknut::CodeGenerator& code, IR::Block block, const EmitConfig& emit_conf) {
    EmittedBlockInfo ebi;

    const std::vector<int> gpr_order{19, 20, 21, 22, 23, 24, 25, 26, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8};
    const std::vector<int> fpr_order{8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    RegAlloc reg_alloc{code, gpr_order, fpr_order};
    EmitContext ctx{block, reg_alloc, emit_conf, ebi};

    ebi.entry_point = code.ptr<CodePtr>();

    for (auto iter = block.begin(); iter != block.end(); ++iter) {
        IR::Inst* inst = &*iter;

        switch (inst->GetOpcode()) {
#define OPCODE(name, type, ...)                    \
    case IR::Opcode::name:                         \
        EmitIR<IR::Opcode::name>(code, ctx, inst); \
        break;
#define A32OPC(name, type, ...)                         \
    case IR::Opcode::A32##name:                         \
        EmitIR<IR::Opcode::A32##name>(code, ctx, inst); \
        break;
#define A64OPC(name, type, ...)                         \
    case IR::Opcode::A64##name:                         \
        EmitIR<IR::Opcode::A64##name>(code, ctx, inst); \
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

    if (emit_conf.enable_cycle_counting) {
        const size_t cycles_to_add = block.CycleCount();
        code.LDR(Xscratch0, SP, offsetof(StackLayout, cycles_remaining));
        if (oaknut::AddSubImm::is_valid(cycles_to_add)) {
            code.SUBS(Xscratch0, Xscratch0, cycles_to_add);
        } else {
            code.MOV(Xscratch1, cycles_to_add);
            code.SUBS(Xscratch0, Xscratch0, Xscratch1);
        }
        code.STR(Xscratch0, SP, offsetof(StackLayout, cycles_remaining));
    }

    EmitA32Terminal(code, ctx);

    ebi.size = code.ptr<CodePtr>() - ebi.entry_point;
    return ebi;
}

void EmitRelocation(oaknut::CodeGenerator& code, EmitContext& ctx, LinkTarget link_target) {
    ctx.ebi.relocations.emplace_back(Relocation{code.ptr<CodePtr>() - ctx.ebi.entry_point, link_target});
    code.NOP();
}

}  // namespace Dynarmic::Backend::Arm64
