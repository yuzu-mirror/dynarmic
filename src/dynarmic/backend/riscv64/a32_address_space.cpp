/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/riscv64/a32_address_space.h"

#include "dynarmic/backend/riscv64/emit_riscv64.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"
#include "dynarmic/frontend/A32/translate/a32_translate.h"
#include "dynarmic/ir/opt/passes.h"

namespace Dynarmic::Backend::RV64 {

A32AddressSpace::A32AddressSpace(const A32::UserConfig& conf)
        : conf(conf)
        , cb(conf.code_cache_size)
        , as(cb.ptr(), conf.code_cache_size) {
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

void* A32AddressSpace::Get(IR::LocationDescriptor descriptor) {
    if (const auto iter = block_entries.find(descriptor.Value()); iter != block_entries.end()) {
        return iter->second;
    }
    return nullptr;
}

void* A32AddressSpace::GetOrEmit(IR::LocationDescriptor descriptor) {
    if (void* block_entry = Get(descriptor)) {
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
    as.RewindBuffer(reinterpret_cast<char*>(prelude_info.end_of_prelude) - reinterpret_cast<char*>(as.GetBufferPointer(0)));
}

void A32AddressSpace::EmitPrelude() {
    using namespace biscuit;
    prelude_info.run_code = reinterpret_cast<PreludeInfo::RunCodeFuncType>(as.GetCursorPointer());

    // TODO: Minimize this.
    as.ADDI(sp, sp, -64 * 8);
    for (std::uint32_t i = 1; i < 32; i += 1) {
        if (GPR{i} == sp || GPR{i} == tp)
            continue;
        as.SD(GPR{i}, i * 8, sp);
    }
    for (std::uint32_t i = 0; i < 32; i += 1) {
        as.FSD(FPR{i}, 32 + i * 8, sp);
    }

    as.JALR(x0, 0, a0);

    prelude_info.return_from_run_code = reinterpret_cast<void*>(as.GetCursorPointer());
    for (std::uint32_t i = 1; i < 32; i += 1) {
        if (GPR{i} == sp || GPR{i} == tp)
            continue;
        as.LD(GPR{i}, i * 8, sp);
    }
    for (std::uint32_t i = 0; i < 32; i += 1) {
        as.FLD(FPR{i}, 32 + i * 8, sp);
    }
    as.ADDI(sp, sp, 64 * 8);
    as.JALR(ra);

    prelude_info.end_of_prelude = reinterpret_cast<u32*>(as.GetCursorPointer());
}

size_t A32AddressSpace::GetRemainingSize() {
    return conf.code_cache_size - (reinterpret_cast<uintptr_t>(as.GetCursorPointer()) - reinterpret_cast<uintptr_t>(as.GetBufferPointer(0)));
}

EmittedBlockInfo A32AddressSpace::Emit(IR::Block block) {
    if (GetRemainingSize() < 1024 * 1024) {
        ClearCache();
    }

    EmittedBlockInfo block_info = EmitRV64(as, std::move(block));
    Link(block_info);

    return block_info;
}

void A32AddressSpace::Link(EmittedBlockInfo& block_info) {
    using namespace biscuit;
    for (auto [ptr_offset, target] : block_info.relocations) {
        Assembler a(reinterpret_cast<u8*>(reinterpret_cast<char*>(block_info.entry_point) + ptr_offset), 4);

        switch (target) {
        case LinkTarget::ReturnFromRunCode: {
            std::ptrdiff_t off = reinterpret_cast<char*>(prelude_info.return_from_run_code) - reinterpret_cast<char*>(a.GetCursorPointer());
            a.JAL(x0, off);
            break;
        }
        default:
            ASSERT_FALSE("Invalid relocation target");
        }
    }
}

}  // namespace Dynarmic::Backend::RV64
