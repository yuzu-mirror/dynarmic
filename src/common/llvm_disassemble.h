/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <string>

#include "common/common_types.h"

namespace Dynarmic::Common {

std::string DisassembleX64(const void* pos, const void* end);
std::string DisassembleAArch32(u32 instruction, u64 pc = 0);
std::string DisassembleAArch64(u32 instruction, u64 pc = 0);

} // namespace Dynarmic::Common
