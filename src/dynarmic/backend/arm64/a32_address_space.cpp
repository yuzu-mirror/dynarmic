/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/arm64/a32_address_space.h"

#include "dynarmic/backend/arm64/a32_jitstate.h"
#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/devirtualize.h"
#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/backend/arm64/stack_layout.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"
#include "dynarmic/frontend/A32/translate/a32_translate.h"
#include "dynarmic/ir/opt/passes.h"

namespace Dynarmic::Backend::Arm64 {

template<auto mfp, typename T>
static void* EmitCallTrampoline(oaknut::CodeGenerator& code, T* this_) {
    using namespace oaknut::util;

    const auto info = Devirtualize<mfp>(this_);

    oaknut::Label l_addr, l_this;

    void* target = code.ptr<void*>();
    code.LDR(X0, l_this);
    code.LDR(Xscratch0, l_addr);
    code.BR(Xscratch0);

    code.align(8);
    code.l(l_this);
    code.dx(info.this_ptr);
    code.l(l_addr);
    code.dx(info.fn_ptr);

    return target;
}

A32AddressSpace::A32AddressSpace(const A32::UserConfig& conf)
        : conf(conf)
        , mem(conf.code_cache_size)
        , code(mem.ptr()) {
    EmitPrelude();
}

IR::Block A32AddressSpace::GenerateIR(IR::LocationDescriptor descriptor) const {
    IR::Block ir_block = A32::Translate(A32::LocationDescriptor{descriptor}, conf.callbacks, {conf.arch_version, conf.define_unpredictable_behaviour, conf.hook_hint_instructions});

    Optimization::PolyfillPass(ir_block, {});
    if (conf.HasOptimization(OptimizationFlag::GetSetElimination)) {
        Optimization::A32GetSetElimination(ir_block, {.convert_nzc_to_nz = true});
        Optimization::DeadCodeElimination(ir_block);
    }
    if (conf.HasOptimization(OptimizationFlag::ConstProp)) {
        Optimization::A32ConstantMemoryReads(ir_block, conf.callbacks);
        Optimization::ConstantPropagation(ir_block);
        Optimization::DeadCodeElimination(ir_block);
    }
    Optimization::VerificationPass(ir_block);

    return ir_block;
}

CodePtr A32AddressSpace::Get(IR::LocationDescriptor descriptor) {
    if (const auto iter = block_entries.find(descriptor.Value()); iter != block_entries.end()) {
        return iter->second;
    }
    return nullptr;
}

CodePtr A32AddressSpace::GetOrEmit(IR::LocationDescriptor descriptor) {
    if (CodePtr block_entry = Get(descriptor)) {
        return block_entry;
    }

    IR::Block ir_block = GenerateIR(descriptor);
    const EmittedBlockInfo block_info = Emit(std::move(ir_block));

    block_infos.insert_or_assign(descriptor.Value(), block_info);
    block_entries.insert_or_assign(descriptor.Value(), block_info.entry_point);
    return block_info.entry_point;
}

void A32AddressSpace::ClearCache() {
    block_entries.clear();
    block_infos.clear();
    code.set_ptr(prelude_info.end_of_prelude);
}

void A32AddressSpace::EmitPrelude() {
    using namespace oaknut::util;

    mem.unprotect();

    prelude_info.run_code = code.ptr<PreludeInfo::RunCodeFuncType>();
    ABI_PushRegisters(code, ABI_CALLEE_SAVE | (1 << 30), sizeof(StackLayout));

    code.MOV(Xstate, X1);
    code.MOV(Xhalt, X2);

    code.LDR(Wscratch0, Xstate, offsetof(A32JitState, upper_location_descriptor));
    code.AND(Wscratch0, Wscratch0, 0xffff0000);
    code.MRS(Xscratch1, oaknut::SystemReg::FPCR);
    code.STR(Wscratch1, SP, offsetof(StackLayout, save_host_fpcr));
    code.MSR(oaknut::SystemReg::FPCR, Xscratch0);

    code.BR(X0);

    prelude_info.return_from_run_code = code.ptr<void*>();

    code.LDR(Wscratch0, SP, offsetof(StackLayout, save_host_fpcr));
    code.MSR(oaknut::SystemReg::FPCR, Xscratch0);

    ABI_PopRegisters(code, ABI_CALLEE_SAVE | (1 << 30), sizeof(StackLayout));
    code.RET();

    prelude_info.read_memory_8 = EmitCallTrampoline<&A32::UserCallbacks::MemoryRead8>(code, conf.callbacks);
    prelude_info.read_memory_16 = EmitCallTrampoline<&A32::UserCallbacks::MemoryRead16>(code, conf.callbacks);
    prelude_info.read_memory_32 = EmitCallTrampoline<&A32::UserCallbacks::MemoryRead32>(code, conf.callbacks);
    prelude_info.read_memory_64 = EmitCallTrampoline<&A32::UserCallbacks::MemoryRead64>(code, conf.callbacks);
    prelude_info.write_memory_8 = EmitCallTrampoline<&A32::UserCallbacks::MemoryWrite8>(code, conf.callbacks);
    prelude_info.write_memory_16 = EmitCallTrampoline<&A32::UserCallbacks::MemoryWrite16>(code, conf.callbacks);
    prelude_info.write_memory_32 = EmitCallTrampoline<&A32::UserCallbacks::MemoryWrite32>(code, conf.callbacks);
    prelude_info.write_memory_64 = EmitCallTrampoline<&A32::UserCallbacks::MemoryWrite64>(code, conf.callbacks);
    prelude_info.isb_raised = EmitCallTrampoline<&A32::UserCallbacks::InstructionSynchronizationBarrierRaised>(code, conf.callbacks);

    prelude_info.end_of_prelude = code.ptr<u32*>();

    mem.invalidate_all();
}

size_t A32AddressSpace::GetRemainingSize() {
    return conf.code_cache_size - (code.ptr<CodePtr>() - reinterpret_cast<CodePtr>(mem.ptr()));
}

EmittedBlockInfo A32AddressSpace::Emit(IR::Block block) {
    if (GetRemainingSize() < 1024 * 1024) {
        ClearCache();
    }

    mem.unprotect();

    const EmitConfig emit_conf{
        .hook_isb = conf.hook_isb,
        .enable_cycle_counting = conf.enable_cycle_counting,
        .always_little_endian = conf.always_little_endian,
        .descriptor_to_fpcr = [](const IR::LocationDescriptor& location) { return FP::FPCR{A32::LocationDescriptor{location}.FPSCR().Value()}; },
        .state_nzcv_offset = offsetof(A32JitState, cpsr_nzcv),
        .state_fpsr_offset = offsetof(A32JitState, fpsr),
        .coprocessors = conf.coprocessors,
    };
    EmittedBlockInfo block_info = EmitArm64(code, std::move(block), emit_conf);

    Link(block_info);

    mem.invalidate(reinterpret_cast<u32*>(block_info.entry_point), block_info.size);
    mem.protect();

    return block_info;
}

void A32AddressSpace::Link(EmittedBlockInfo& block_info) {
    using namespace oaknut;
    using namespace oaknut::util;

    for (auto [ptr_offset, target] : block_info.relocations) {
        CodeGenerator c{reinterpret_cast<u32*>(block_info.entry_point + ptr_offset)};

        switch (target) {
        case LinkTarget::ReturnFromRunCode:
            c.B(prelude_info.return_from_run_code);
            break;
        case LinkTarget::ReadMemory8:
            c.BL(prelude_info.read_memory_8);
            break;
        case LinkTarget::ReadMemory16:
            c.BL(prelude_info.read_memory_16);
            break;
        case LinkTarget::ReadMemory32:
            c.BL(prelude_info.read_memory_32);
            break;
        case LinkTarget::ReadMemory64:
            c.BL(prelude_info.read_memory_64);
            break;
        case LinkTarget::WriteMemory8:
            c.BL(prelude_info.write_memory_8);
            break;
        case LinkTarget::WriteMemory16:
            c.BL(prelude_info.write_memory_16);
            break;
        case LinkTarget::WriteMemory32:
            c.BL(prelude_info.write_memory_32);
            break;
        case LinkTarget::WriteMemory64:
            c.BL(prelude_info.write_memory_64);
            break;
        case LinkTarget::InstructionSynchronizationBarrierRaised:
            c.BL(prelude_info.isb_raised);
            break;
        default:
            ASSERT_FALSE("Invalid relocation target");
        }
    }
}

}  // namespace Dynarmic::Backend::Arm64
