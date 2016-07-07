/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <memory>

#include "backend_x64/emit_x64.h"
#include "backend_x64/jitstate.h"
#include "backend_x64/routines.h"
#include "common/assert.h"
#include "common/bit_util.h"
#include "common/common_types.h"
#include "common/scope_exit.h"
#include "frontend/arm_types.h"
#include "frontend/translate.h"
#include "interface/interface.h"

namespace Dynarmic {

using namespace BackendX64;

struct BlockOfCode : Gen::XCodeBlock {
    BlockOfCode() {
        AllocCodeSpace(128 * 1024 * 1024);
    }
};

struct Jit::Impl {
    Impl(Jit* jit, UserCallbacks callbacks) : emitter(&block_of_code, &routines, callbacks, jit), callbacks(callbacks) {}

    JitState jit_state{};
    Routines routines{};
    BlockOfCode block_of_code{};
    EmitX64 emitter;
    const UserCallbacks callbacks;

    size_t Execute(size_t cycle_count) {
        u32 pc = jit_state.Reg[15];
        bool TFlag = Common::Bit<5>(jit_state.Cpsr);
        bool EFlag = Common::Bit<9>(jit_state.Cpsr);

        Arm::LocationDescriptor descriptor{pc, TFlag, EFlag};

        CodePtr code_ptr = GetBasicBlock(descriptor);
        return routines.RunCode(&jit_state, code_ptr, cycle_count);
    }
private:
    CodePtr GetBasicBlock(Arm::LocationDescriptor descriptor) {
        CodePtr code_ptr = emitter.GetBasicBlock(descriptor);
        if (code_ptr)
            return code_ptr;

        IR::Block ir_block = Arm::Translate(descriptor, callbacks.MemoryRead32);
        return emitter.Emit(descriptor, ir_block);
    }
};

Jit::Jit(UserCallbacks callbacks) : callbacks(callbacks), impl(std::make_unique<Impl>(this, callbacks)) {}

Jit::~Jit() {}

size_t Jit::Run(size_t cycle_count) {
    ASSERT(!is_executing);
    is_executing = true;
    SCOPE_EXIT({ this->is_executing = false; });

    halt_requested = false;

    size_t cycles_executed = 0;
    while (cycles_executed < cycle_count && !halt_requested) {
        cycles_executed += impl->Execute(cycle_count - cycles_executed);
    }

    return cycles_executed;
}

void Jit::ClearCache(bool poison_memory) {
    ASSERT(!is_executing);
}

void Jit::HaltExecution() {
    ASSERT(is_executing);
    halt_requested = true;

    // TODO: Uh do other stuff to JitState pls.
}

std::array<u32, 16>& Jit::Regs() {
    return impl->jit_state.Reg;
}
std::array<u32, 16> Jit::Regs() const {
    return impl->jit_state.Reg;
}

u32& Jit::Cpsr() {
    return impl->jit_state.Cpsr;
}
u32 Jit::Cpsr() const {
    return impl->jit_state.Cpsr;
}

} // namespace Dynarmic
