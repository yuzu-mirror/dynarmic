/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <biscuit/assembler.hpp>
#include <tsl/robin_map.h>

#include "dynarmic/backend/riscv64/code_block.h"
#include "dynarmic/backend/riscv64/emit_riscv64.h"
#include "dynarmic/interface/A32/config.h"
#include "dynarmic/interface/halt_reason.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/location_descriptor.h"

namespace Dynarmic::Backend::RV64 {

struct A32JitState;

class A32AddressSpace final {
public:
    explicit A32AddressSpace(const A32::UserConfig& conf);

    IR::Block GenerateIR(IR::LocationDescriptor) const;

    void* Get(IR::LocationDescriptor descriptor);

    void* GetOrEmit(IR::LocationDescriptor descriptor);

    void ClearCache();

private:
    friend class A32Core;

    void EmitPrelude();

    size_t GetRemainingSize();
    EmittedBlockInfo Emit(IR::Block ir_block);
    void Link(EmittedBlockInfo& block);

    const A32::UserConfig conf;

    CodeBlock cb;
    biscuit::Assembler as;

    tsl::robin_map<u64, void*> block_entries;
    tsl::robin_map<u64, EmittedBlockInfo> block_infos;

    struct PreludeInfo {
        u32* end_of_prelude;

        using RunCodeFuncType = HaltReason (*)(void* entry_point, A32JitState* context, volatile u32* halt_reason);
        RunCodeFuncType run_code;
        void* return_from_run_code;
    } prelude_info;
};

}  // namespace Dynarmic::Backend::RV64
