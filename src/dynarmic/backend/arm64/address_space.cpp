/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/arm64/a64_address_space.h"
#include "dynarmic/backend/arm64/a64_jitstate.h"
#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/devirtualize.h"
#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/backend/arm64/stack_layout.h"
#include "dynarmic/common/cast_util.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/interface/exclusive_monitor.h"
#include "dynarmic/ir/opt/passes.h"

namespace Dynarmic::Backend::Arm64 {

AddressSpace::AddressSpace(size_t code_cache_size)
        : code_cache_size(code_cache_size)
        , mem(code_cache_size)
        , code(mem.ptr()) {}

AddressSpace::~AddressSpace() = default;

CodePtr AddressSpace::Get(IR::LocationDescriptor descriptor) {
    if (const auto iter = block_entries.find(descriptor.Value()); iter != block_entries.end()) {
        return iter->second;
    }
    return nullptr;
}

CodePtr AddressSpace::GetOrEmit(IR::LocationDescriptor descriptor) {
    if (CodePtr block_entry = Get(descriptor)) {
        return block_entry;
    }

    IR::Block ir_block = GenerateIR(descriptor);
    const EmittedBlockInfo block_info = Emit(std::move(ir_block));

    block_infos.insert_or_assign(descriptor.Value(), block_info);
    block_entries.insert_or_assign(descriptor.Value(), block_info.entry_point);
    return block_info.entry_point;
}

void AddressSpace::ClearCache() {
    block_entries.clear();
    block_infos.clear();
    block_references.clear();
    code.set_ptr(prelude_info.end_of_prelude);
}

size_t AddressSpace::GetRemainingSize() {
    return code_cache_size - (code.ptr<CodePtr>() - reinterpret_cast<CodePtr>(mem.ptr()));
}

EmittedBlockInfo AddressSpace::Emit(IR::Block block) {
    if (GetRemainingSize() < 1024 * 1024) {
        ClearCache();
    }

    mem.unprotect();

    EmittedBlockInfo block_info = EmitArm64(code, std::move(block), GetEmitConfig());

    Link(block.Location(), block_info);

    mem.invalidate(reinterpret_cast<u32*>(block_info.entry_point), block_info.size);

    RelinkForDescriptor(block.Location());

    mem.protect();

    return block_info;
}

static void LinkBlockLinks(const CodePtr entry_point, const CodePtr target_ptr, const std::vector<BlockRelocation>& block_relocations_list) {
    using namespace oaknut;
    using namespace oaknut::util;

    for (auto [ptr_offset] : block_relocations_list) {
        CodeGenerator c{reinterpret_cast<u32*>(entry_point + ptr_offset)};

        if (target_ptr) {
            c.B((void*)target_ptr);
        } else {
            c.NOP();
        }
    }
}

void AddressSpace::Link(IR::LocationDescriptor block_descriptor, EmittedBlockInfo& block_info) {
    using namespace oaknut;
    using namespace oaknut::util;

    for (auto [ptr_offset, target] : block_info.relocations) {
        CodeGenerator c{reinterpret_cast<u32*>(block_info.entry_point + ptr_offset)};

        switch (target) {
        case LinkTarget::ReturnToDispatcher:
            c.B(prelude_info.return_to_dispatcher);
            break;
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
        case LinkTarget::ReadMemory128:
            c.BL(prelude_info.read_memory_128);
            break;
        case LinkTarget::ExclusiveReadMemory8:
            c.BL(prelude_info.exclusive_read_memory_8);
            break;
        case LinkTarget::ExclusiveReadMemory16:
            c.BL(prelude_info.exclusive_read_memory_16);
            break;
        case LinkTarget::ExclusiveReadMemory32:
            c.BL(prelude_info.exclusive_read_memory_32);
            break;
        case LinkTarget::ExclusiveReadMemory64:
            c.BL(prelude_info.exclusive_read_memory_64);
            break;
        case LinkTarget::ExclusiveReadMemory128:
            c.BL(prelude_info.exclusive_read_memory_128);
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
        case LinkTarget::WriteMemory128:
            c.BL(prelude_info.write_memory_128);
            break;
        case LinkTarget::ExclusiveWriteMemory8:
            c.BL(prelude_info.exclusive_write_memory_8);
            break;
        case LinkTarget::ExclusiveWriteMemory16:
            c.BL(prelude_info.exclusive_write_memory_16);
            break;
        case LinkTarget::ExclusiveWriteMemory32:
            c.BL(prelude_info.exclusive_write_memory_32);
            break;
        case LinkTarget::ExclusiveWriteMemory64:
            c.BL(prelude_info.exclusive_write_memory_64);
            break;
        case LinkTarget::ExclusiveWriteMemory128:
            c.BL(prelude_info.exclusive_write_memory_128);
            break;
        case LinkTarget::CallSVC:
            c.BL(prelude_info.call_svc);
            break;
        case LinkTarget::ExceptionRaised:
            c.BL(prelude_info.exception_raised);
            break;
        case LinkTarget::InstructionSynchronizationBarrierRaised:
            c.BL(prelude_info.isb_raised);
            break;
        case LinkTarget::InstructionCacheOperationRaised:
            c.BL(prelude_info.ic_raised);
            break;
        case LinkTarget::DataCacheOperationRaised:
            c.BL(prelude_info.dc_raised);
            break;
        case LinkTarget::GetCNTPCT:
            c.BL(prelude_info.get_cntpct);
            break;
        case LinkTarget::AddTicks:
            c.BL(prelude_info.add_ticks);
            break;
        case LinkTarget::GetTicksRemaining:
            c.BL(prelude_info.get_ticks_remaining);
            break;
        default:
            ASSERT_FALSE("Invalid relocation target");
        }
    }

    for (auto [target_descriptor, list] : block_info.block_relocations) {
        block_references[target_descriptor.Value()].emplace(block_descriptor.Value());
        LinkBlockLinks(block_info.entry_point, Get(target_descriptor), list);
    }
}

void AddressSpace::RelinkForDescriptor(IR::LocationDescriptor target_descriptor) {
    for (auto block_descriptor : block_references[target_descriptor.Value()]) {
        if (auto iter = block_infos.find(block_descriptor); iter != block_infos.end()) {
            const EmittedBlockInfo& block_info = iter->second;

            LinkBlockLinks(block_info.entry_point, Get(target_descriptor), block_infos[block_descriptor].block_relocations[target_descriptor]);

            mem.invalidate(reinterpret_cast<u32*>(block_info.entry_point), block_info.size);
        }
    }
}

}  // namespace Dynarmic::Backend::Arm64
