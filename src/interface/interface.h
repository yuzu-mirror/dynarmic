/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "common/common_types.h"

namespace Dynarmic {

struct UserCallbacks {
    u8 (*MemoryRead8)(u32 vaddr);
    u16 (*MemoryRead16)(u32 vaddr);
    u32 (*MemoryRead32)(u32 vaddr);
    u64 (*MemoryRead64)(u32 vaddr);

    void (*MemoryWrite8)(u32 vaddr, u8 value);
    void (*MemoryWrite16)(u32 vaddr, u16 value);
    void (*MemoryWrite32)(u32 vaddr, u32 value);
    void (*MemoryWrite64)(u32 vaddr, u64 value);

    void (*InterpreterFallback)(u32 pc, void* jit_state);

    bool (*SoftwareInterrupt)(u32 swi);
};

} // namespace Dynarmic
