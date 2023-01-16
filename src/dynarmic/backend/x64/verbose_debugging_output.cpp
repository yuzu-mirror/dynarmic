/* This file is part of the dynarmic project.
 * Copyright (c) 2023 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/verbose_debugging_output.h"

#include <fmt/format.h>

#include "dynarmic/backend/x64/hostloc.h"

namespace Dynarmic::Backend::X64 {

void PrintVerboseDebuggingOutputLine(RegisterData& reg_data, HostLoc hostloc, u64 inst_addr) {
    if (HostLocIsGPR(hostloc)) {
        const u64 value = reg_data.gprs[HostLocToReg64(hostloc).getIdx()];
        fmt::print("dynarmic debug: {:016x} = {:016x}{:016x}\n", inst_addr, 0, value);
    } else if (HostLocIsXMM(hostloc)) {
        const Vector value = reg_data.xmms[HostLocToXmm(hostloc).getIdx()];
        fmt::print("dynarmic debug: {:016x} = {:016x}{:016x}\n", inst_addr, value[1], value[0]);
    } else if (HostLocIsSpill(hostloc)) {
        const Vector value = (*reg_data.spill)[static_cast<size_t>(hostloc) - static_cast<size_t>(HostLoc::FirstSpill)];
        fmt::print("dynarmic debug: {:016x} = {:016x}{:016x}\n", inst_addr, value[1], value[0]);
    } else {
        fmt::print("dynarmic debug: Invalid hostloc\n");
    }
}

}  // namespace Dynarmic::Backend::X64
