/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <xbyak/xbyak.h>

#include "dynarmic/backend/x64/a64_emit_x64.h"
#include "dynarmic/backend/x64/exclusive_monitor_friend.h"
#include "dynarmic/common/spin_lock_x64.h"
#include "dynarmic/interface/exclusive_monitor.h"

namespace Dynarmic::Backend::X64 {

namespace {

using namespace Xbyak::util;

constexpr size_t page_bits = 12;
constexpr size_t page_size = 1 << page_bits;
constexpr size_t page_mask = (1 << page_bits) - 1;

template<typename EmitContext>
void EmitDetectMisalignedVAddr(BlockOfCode& code, EmitContext& ctx, size_t bitsize, Xbyak::Label& abort, Xbyak::Reg64 vaddr, Xbyak::Reg64 tmp) {
    if (bitsize == 8 || (ctx.conf.detect_misaligned_access_via_page_table & bitsize) == 0) {
        return;
    }

    const u32 align_mask = [bitsize]() -> u32 {
        switch (bitsize) {
        case 16:
            return 0b1;
        case 32:
            return 0b11;
        case 64:
            return 0b111;
        case 128:
            return 0b1111;
        default:
            UNREACHABLE();
        }
    }();

    code.test(vaddr, align_mask);

    if (!ctx.conf.only_detect_misalignment_via_page_table_on_page_boundary) {
        code.jnz(abort, code.T_NEAR);
        return;
    }

    const u32 page_align_mask = static_cast<u32>(page_size - 1) & ~align_mask;

    Xbyak::Label detect_boundary, resume;

    code.jnz(detect_boundary, code.T_NEAR);
    code.L(resume);

    code.SwitchToFarCode();
    code.L(detect_boundary);
    code.mov(tmp, vaddr);
    code.and_(tmp, page_align_mask);
    code.cmp(tmp, page_align_mask);
    code.jne(resume, code.T_NEAR);
    // NOTE: We expect to fallthrough into abort code here.
    code.SwitchToNearCode();
}

template<typename UserConfig>
void EmitExclusiveLock(BlockOfCode& code, const UserConfig& conf, Xbyak::Reg64 pointer, Xbyak::Reg32 tmp) {
    if (conf.HasOptimization(OptimizationFlag::Unsafe_IgnoreGlobalMonitor)) {
        return;
    }

    code.mov(pointer, Common::BitCast<u64>(GetExclusiveMonitorLockPointer(conf.global_monitor)));
    EmitSpinLockLock(code, pointer, tmp);
}

template<typename UserConfig>
void EmitExclusiveUnlock(BlockOfCode& code, const UserConfig& conf, Xbyak::Reg64 pointer, Xbyak::Reg32 tmp) {
    if (conf.HasOptimization(OptimizationFlag::Unsafe_IgnoreGlobalMonitor)) {
        return;
    }

    code.mov(pointer, Common::BitCast<u64>(GetExclusiveMonitorLockPointer(conf.global_monitor)));
    EmitSpinLockUnlock(code, pointer, tmp);
}

template<typename UserConfig>
void EmitExclusiveTestAndClear(BlockOfCode& code, const UserConfig& conf, Xbyak::Reg64 vaddr, Xbyak::Reg64 pointer, Xbyak::Reg64 tmp) {
    if (conf.HasOptimization(OptimizationFlag::Unsafe_IgnoreGlobalMonitor)) {
        return;
    }

    code.mov(tmp, 0xDEAD'DEAD'DEAD'DEAD);
    const size_t processor_count = GetExclusiveMonitorProcessorCount(conf.global_monitor);
    for (size_t processor_index = 0; processor_index < processor_count; processor_index++) {
        if (processor_index == conf.processor_id) {
            continue;
        }
        Xbyak::Label ok;
        code.mov(pointer, Common::BitCast<u64>(GetExclusiveMonitorAddressPointer(conf.global_monitor, processor_index)));
        code.cmp(qword[pointer], vaddr);
        code.jne(ok);
        code.mov(qword[pointer], tmp);
        code.L(ok);
    }
}

}  // namespace

}  // namespace Dynarmic::Backend::X64
