/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <xbyak.h>

#include "backend/x64/abi.h"
#include "backend/x64/hostloc.h"
#include "backend/x64/stack_layout.h"

namespace Dynarmic::Backend::X64 {

Xbyak::Reg64 HostLocToReg64(HostLoc loc) {
    ASSERT(HostLocIsGPR(loc));
    return Xbyak::Reg64(static_cast<int>(loc));
}

Xbyak::Xmm HostLocToXmm(HostLoc loc) {
    ASSERT(HostLocIsXMM(loc));
    return Xbyak::Xmm(static_cast<int>(loc) - static_cast<int>(HostLoc::XMM0));
}

Xbyak::Address SpillToOpArg(HostLoc loc) {
    ASSERT(HostLocIsSpill(loc));

    size_t i = static_cast<size_t>(loc) - static_cast<size_t>(HostLoc::FirstSpill);
    ASSERT_MSG(i < SpillCount, "Spill index greater than number of available spill locations");

    using namespace Xbyak::util;
    return xword[rsp + ABI_SHADOW_SPACE + offsetof(StackLayout, spill) + i * sizeof(u64) * 2];
}

} // namespace Dynarmic::Backend::X64
