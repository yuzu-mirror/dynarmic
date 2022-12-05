/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/a32_jitstate.h"
#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/backend/arm64/emit_arm64_memory.h"
#include "dynarmic/backend/arm64/emit_context.h"
#include "dynarmic/backend/arm64/reg_alloc.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

template<>
void EmitIR<IR::Opcode::A32ClearExclusive>(oaknut::CodeGenerator& code, EmitContext&, IR::Inst*) {
    code.STR(WZR, Xstate, offsetof(A32JitState, exclusive_state));
}

template<>
void EmitIR<IR::Opcode::A32ReadMemory8>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitReadMemory(code, ctx, inst, LinkTarget::ReadMemory8);
}

template<>
void EmitIR<IR::Opcode::A32ReadMemory16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitReadMemory(code, ctx, inst, LinkTarget::ReadMemory16);
}

template<>
void EmitIR<IR::Opcode::A32ReadMemory32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitReadMemory(code, ctx, inst, LinkTarget::ReadMemory32);
}

template<>
void EmitIR<IR::Opcode::A32ReadMemory64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitReadMemory(code, ctx, inst, LinkTarget::ReadMemory64);
}

template<>
void EmitIR<IR::Opcode::A32ExclusiveReadMemory8>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveReadMemory(code, ctx, inst, LinkTarget::ExclusiveReadMemory8);
}

template<>
void EmitIR<IR::Opcode::A32ExclusiveReadMemory16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveReadMemory(code, ctx, inst, LinkTarget::ExclusiveReadMemory16);
}

template<>
void EmitIR<IR::Opcode::A32ExclusiveReadMemory32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveReadMemory(code, ctx, inst, LinkTarget::ExclusiveReadMemory32);
}

template<>
void EmitIR<IR::Opcode::A32ExclusiveReadMemory64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveReadMemory(code, ctx, inst, LinkTarget::ExclusiveReadMemory64);
}

template<>
void EmitIR<IR::Opcode::A32WriteMemory8>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitWriteMemory(code, ctx, inst, LinkTarget::WriteMemory8);
}

template<>
void EmitIR<IR::Opcode::A32WriteMemory16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitWriteMemory(code, ctx, inst, LinkTarget::WriteMemory16);
}

template<>
void EmitIR<IR::Opcode::A32WriteMemory32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitWriteMemory(code, ctx, inst, LinkTarget::WriteMemory32);
}

template<>
void EmitIR<IR::Opcode::A32WriteMemory64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitWriteMemory(code, ctx, inst, LinkTarget::WriteMemory64);
}

template<>
void EmitIR<IR::Opcode::A32ExclusiveWriteMemory8>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveWriteMemory(code, ctx, inst, LinkTarget::ExclusiveWriteMemory8);
}

template<>
void EmitIR<IR::Opcode::A32ExclusiveWriteMemory16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveWriteMemory(code, ctx, inst, LinkTarget::ExclusiveWriteMemory16);
}

template<>
void EmitIR<IR::Opcode::A32ExclusiveWriteMemory32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveWriteMemory(code, ctx, inst, LinkTarget::ExclusiveWriteMemory32);
}

template<>
void EmitIR<IR::Opcode::A32ExclusiveWriteMemory64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitExclusiveWriteMemory(code, ctx, inst, LinkTarget::ExclusiveWriteMemory64);
}

}  // namespace Dynarmic::Backend::Arm64
