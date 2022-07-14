/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/arm64/a32_address_space.h"

#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"
#include "dynarmic/frontend/A32/translate/a32_translate.h"
#include "dynarmic/ir/opt/passes.h"

namespace Dynarmic::Backend::Arm64 {

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
    code.set_ptr(prelude_info.end_of_prelude);
}

void A32AddressSpace::EmitPrelude() {
    using namespace oaknut;
    using namespace oaknut::util;

    mem.unprotect();

    prelude_info.run_code = code.ptr<PreludeInfo::RunCodeFuncType>();
    // TODO: Minimize this.
    code.STR(X30, SP, PRE_INDEXED, -16);
    for (int i = 0; i < 30; i += 2) {
        code.STP(XReg{i}, XReg{i + 1}, SP, PRE_INDEXED, -16);
    }
    for (int i = 0; i < 32; i += 2) {
        code.STP(QReg{i}, QReg{i + 1}, SP, PRE_INDEXED, -32);
    }
    code.BR(X0);

    prelude_info.return_from_run_code = code.ptr<void*>();
    for (int i = 30; i >= 0; i -= 2) {
        code.LDP(QReg{i}, QReg{i + 1}, SP, POST_INDEXED, 32);
    }
    for (int i = 28; i >= 0; i -= 2) {
        code.LDP(XReg{i}, XReg{i + 1}, SP, POST_INDEXED, 16);
    }
    code.LDR(X30, SP, POST_INDEXED, 16);
    code.RET();

    mem.protect();

    prelude_info.end_of_prelude = code.ptr<u32*>();
}

size_t A32AddressSpace::GetRemainingSize() {
    return conf.code_cache_size - (reinterpret_cast<uintptr_t>(code.ptr<void*>()) - reinterpret_cast<uintptr_t>(mem.ptr()));
}

EmittedBlockInfo A32AddressSpace::Emit(IR::Block block) {
    if (GetRemainingSize() < 1024 * 1024) {
        ClearCache();
    }

    mem.unprotect();

    EmittedBlockInfo block_info = EmitArm64(code, std::move(block));
    Link(block_info);

    mem.protect();

    return block_info;
}

void A32AddressSpace::Link(EmittedBlockInfo& block_info) {
    using namespace oaknut;
    using namespace oaknut::util;

    for (auto [ptr_offset, target] : block_info.relocations) {
        CodeGenerator c{reinterpret_cast<u32*>(reinterpret_cast<char*>(block_info.entry_point) + ptr_offset)};

        switch (target) {
        case LinkTarget::ReturnFromRunCode:
            c.B(prelude_info.return_from_run_code);
            break;
        default:
            ASSERT_FALSE("Invalid relocation target");
        }
    }
}

}  // namespace Dynarmic::Backend::Arm64
