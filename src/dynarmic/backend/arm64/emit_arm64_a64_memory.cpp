/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/a64_jitstate.h"
#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/backend/arm64/emit_arm64_memory.h"
#include "dynarmic/backend/arm64/emit_context.h"
#include "dynarmic/backend/arm64/reg_alloc.h"
#include "dynarmic/ir/acc_type.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

template<>
void EmitIR<IR::Opcode::A64ClearExclusive>(oaknut::CodeGenerator& code, EmitContext&, IR::Inst*) {
    code.STR(WZR, Xstate, offsetof(A64JitState, exclusive_state));
}

template<>
void EmitIR<IR::Opcode::A64ReadMemory8>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitReadMemory(code, ctx, inst, LinkTarget::ReadMemory8);
}

template<>
void EmitIR<IR::Opcode::A64ReadMemory16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitReadMemory(code, ctx, inst, LinkTarget::ReadMemory16);
}

template<>
void EmitIR<IR::Opcode::A64ReadMemory32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitReadMemory(code, ctx, inst, LinkTarget::ReadMemory32);
}

template<>
void EmitIR<IR::Opcode::A64ReadMemory64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitReadMemory(code, ctx, inst, LinkTarget::ReadMemory64);
}

template<>
void EmitIR<IR::Opcode::A64ReadMemory128>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitReadMemory128(code, ctx, inst, LinkTarget::ReadMemory128);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveReadMemory8>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveReadMemory(code, ctx, inst, LinkTarget::ExclusiveReadMemory8);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveReadMemory16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveReadMemory(code, ctx, inst, LinkTarget::ExclusiveReadMemory16);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveReadMemory32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveReadMemory(code, ctx, inst, LinkTarget::ExclusiveReadMemory32);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveReadMemory64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveReadMemory(code, ctx, inst, LinkTarget::ExclusiveReadMemory64);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveReadMemory128>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveReadMemory128(code, ctx, inst, LinkTarget::ExclusiveReadMemory128);
}

template<>
void EmitIR<IR::Opcode::A64WriteMemory8>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitWriteMemory(code, ctx, inst, LinkTarget::WriteMemory8);
}

template<>
void EmitIR<IR::Opcode::A64WriteMemory16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitWriteMemory(code, ctx, inst, LinkTarget::WriteMemory16);
}

template<>
void EmitIR<IR::Opcode::A64WriteMemory32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitWriteMemory(code, ctx, inst, LinkTarget::WriteMemory32);
}

template<>
void EmitIR<IR::Opcode::A64WriteMemory64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitWriteMemory(code, ctx, inst, LinkTarget::WriteMemory64);
}

template<>
void EmitIR<IR::Opcode::A64WriteMemory128>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitWriteMemory(code, ctx, inst, LinkTarget::WriteMemory128);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveWriteMemory8>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveWriteMemory(code, ctx, inst, LinkTarget::ExclusiveWriteMemory8);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveWriteMemory16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveWriteMemory(code, ctx, inst, LinkTarget::ExclusiveWriteMemory16);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveWriteMemory32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveWriteMemory(code, ctx, inst, LinkTarget::ExclusiveWriteMemory32);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveWriteMemory64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveWriteMemory(code, ctx, inst, LinkTarget::ExclusiveWriteMemory64);
}

template<>
void EmitIR<IR::Opcode::A64ExclusiveWriteMemory128>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveWriteMemory(code, ctx, inst, LinkTarget::ExclusiveWriteMemory128);
}

}  // namespace Dynarmic::Backend::Arm64
