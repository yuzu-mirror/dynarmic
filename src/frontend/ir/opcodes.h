/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "common/common_types.h"

namespace Dynarmic {
namespace IR {

enum class Opcode {
#define OPCODE(name, type, ...) name,
#include "opcodes.inc"
#undef OPCODE
    NUM_OPCODE
};

constexpr size_t OpcodeCount = static_cast<size_t>(Opcode::NUM_OPCODE);

} // namespace Arm
} // namespace Dynarmic
