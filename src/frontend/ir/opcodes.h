/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "common/common_types.h"

namespace Dynarmic {
namespace IR {

/**
 * The Opcodes of our intermediate representation.
 * Type signatures for each opcode can be found in opcodes.inc
 */
enum class Opcode {
#define OPCODE(name, type, ...) name,
#include "opcodes.inc"
#undef OPCODE
    NUM_OPCODE
};

constexpr size_t OpcodeCount = static_cast<size_t>(Opcode::NUM_OPCODE);

/**
 * The intermediate representation is typed. These are the used by our IR.
 */
enum class Type {
    Void      = 1 << 0,
    RegRef    = 1 << 1,
    ExtRegRef = 1 << 2,
    Opaque    = 1 << 3,
    U1        = 1 << 4,
    U8        = 1 << 5,
    U16       = 1 << 6,
    U32       = 1 << 7,
    U64       = 1 << 8,
    F32       = 1 << 9,
    F64       = 1 << 10,
};

/// Get return type of an opcode
Type GetTypeOf(Opcode op);

/// Get the number of arguments an opcode accepts
size_t GetNumArgsOf(Opcode op);

/// Get the required type of an argument of an opcode
Type GetArgTypeOf(Opcode op, size_t arg_index);

/// Get the name of an opcode.
const char* GetNameOf(Opcode op);

} // namespace Arm
} // namespace Dynarmic
