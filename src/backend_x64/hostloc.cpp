/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "backend_x64/hostloc.h"

namespace Dynarmic {
namespace BackendX64 {

Xbyak::Reg64 HostLocToReg64(HostLoc loc) {
    DEBUG_ASSERT(HostLocIsGPR(loc));
    return Xbyak::Reg64(static_cast<int>(loc));
}

Xbyak::Xmm HostLocToXmm(HostLoc loc) {
    DEBUG_ASSERT(HostLocIsXMM(loc));
    return Xbyak::Xmm(static_cast<int>(loc) - static_cast<int>(HostLoc::XMM0));
}

Xbyak::Address SpillToOpArg(HostLoc loc) {
    using namespace Xbyak::util;

    static_assert(std::is_same<decltype(JitState::Spill[0]), u64&>::value, "Spill must be u64");
    DEBUG_ASSERT(HostLocIsSpill(loc));

    size_t i = static_cast<size_t>(loc) - static_cast<size_t>(HostLoc::FirstSpill);
    return qword[r15 + offsetof(JitState, Spill) + i * sizeof(u64)];
}

} // namespace BackendX64
} // namespace Dynarmic
