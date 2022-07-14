/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/stdint.hpp>
#include <oaknut/code_block.hpp>
#include <oaknut/oaknut.hpp>
#include <tsl/robin_map.h>

#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/interface/A32/config.h"
#include "dynarmic/interface/halt_reason.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/location_descriptor.h"

namespace Dynarmic::Backend::Arm64 {

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

    oaknut::CodeBlock mem;
    oaknut::CodeGenerator code;

    tsl::robin_map<u64, void*> block_entries;
    tsl::robin_map<u64, EmittedBlockInfo> block_infos;

    struct PreludeInfo {
        u32* end_of_prelude;

        using RunCodeFuncType = HaltReason (*)(void* entry_point, A32JitState* context, volatile u32* halt_reason);
        RunCodeFuncType run_code;
        void* return_from_run_code;
    } prelude_info;
};

}  // namespace Dynarmic::Backend::Arm64
