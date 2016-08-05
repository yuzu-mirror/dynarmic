/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <memory>

#ifdef DYNARMIC_USE_LLVM
#include <llvm-c/Disassembler.h>
#include <llvm-c/Target.h>
#endif

#include "backend_x64/emit_x64.h"
#include "backend_x64/jitstate.h"
#include "backend_x64/routines.h"
#include "common/assert.h"
#include "common/bit_util.h"
#include "common/common_types.h"
#include "common/scope_exit.h"
#include "common/string_util.h"
#include "frontend/arm_types.h"
#include "frontend/translate/translate.h"
#include "interface/interface.h"
#include "ir_opt/passes.h"

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

        Arm::LocationDescriptor descriptor{pc, TFlag, EFlag, jit_state.guest_FPSCR_flags};

        CodePtr code_ptr = GetBasicBlock(descriptor)->code_ptr;
        return routines.RunCode(&jit_state, code_ptr, cycle_count);
    }

    std::string Disassemble(Arm::LocationDescriptor descriptor) {
        auto block = GetBasicBlock(descriptor);
        std::string result = Common::StringFromFormat("address: %p\nsize: %zu bytes\n", block->code_ptr, block->size);

#ifdef DYNARMIC_USE_LLVM
        CodePtr end = block->code_ptr + block->size;
        size_t remaining = block->size;

        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86TargetMC();
        LLVMInitializeX86Disassembler();
        LLVMDisasmContextRef llvm_ctx = LLVMCreateDisasm("x86_64", nullptr, 0, nullptr, nullptr);
        LLVMSetDisasmOptions(llvm_ctx, LLVMDisassembler_Option_AsmPrinterVariant);

        for (CodePtr pos = block->code_ptr; pos < end;) {
            char buffer[80];
            size_t inst_size = LLVMDisasmInstruction(llvm_ctx, const_cast<u8*>(pos), remaining, (u64)pos, buffer, sizeof(buffer));
            assert(inst_size);
            for (CodePtr i = pos; i < pos + inst_size; i++)
                result.append(Common::StringFromFormat("%02x ", *i));
            for (size_t i = inst_size; i < 10; i++)
                result.append("   ");
            result.append(buffer);
            result.append("\n");

            pos += inst_size;
            remaining -= inst_size;
        }

        LLVMDisasmDispose(llvm_ctx);
#else
        result.append("(recompile with DYNARMIC_USE_LLVM=ON to disassemble the generated x86_64 code)\n");
#endif

        return result;
    }

private:
    EmitX64::BlockDescriptor* GetBasicBlock(Arm::LocationDescriptor descriptor) {
        auto block = emitter.GetBasicBlock(descriptor);
        if (block)
            return block;

        IR::Block ir_block = Arm::Translate(descriptor, callbacks.MemoryRead32);
        Optimization::GetSetElimination(ir_block);
        Optimization::DeadCodeElimination(ir_block);
        Optimization::VerificationPass(ir_block);
        return emitter.Emit(descriptor, ir_block);
    }
};

Jit::Jit(UserCallbacks callbacks) : impl(std::make_unique<Impl>(this, callbacks)) {}

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

    if (poison_memory) {
        impl->block_of_code.ClearCodeSpace();
    } else {
        impl->block_of_code.ResetCodePtr();
    }

    impl->emitter.ClearCache();
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

std::array<u32, 64>& Jit::ExtRegs() {
    return impl->jit_state.ExtReg;
}

std::array<u32, 64> Jit::ExtRegs() const {
    return impl->jit_state.ExtReg;
}

u32& Jit::Cpsr() {
    return impl->jit_state.Cpsr;
}

u32 Jit::Cpsr() const {
    return impl->jit_state.Cpsr;
}

u32 Jit::Fpscr() const {
    return impl->jit_state.Fpscr();
}

void Jit::SetFpscr(u32 value) const {
    return impl->jit_state.SetFpscr(value);
}

std::string Jit::Disassemble(Arm::LocationDescriptor descriptor) {
    return impl->Disassemble(descriptor);
}

} // namespace Dynarmic
