/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <string>

#include "common/common_types.h"

namespace Dynarmic {
namespace Arm {

std::string DisassembleArm(u32 instruction);

} // namespace Arm
} // namespace Dynarmic
