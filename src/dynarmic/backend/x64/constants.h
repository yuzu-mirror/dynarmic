/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "dynarmic/common/common_types.h"

namespace Dynarmic::Backend::X64 {

// Redefinition of _MM_CMPINT_* constants for use with the 'vpcmp' instruction
namespace CmpInt {
constexpr u8 Equal = 0x0;
constexpr u8 LessThan = 0x1;
constexpr u8 LessEqual = 0x2;
constexpr u8 False = 0x3;
constexpr u8 NotEqual = 0x4;
constexpr u8 NotLessThan = 0x5;
constexpr u8 GreaterEqual = 0x5;
constexpr u8 NotLessEqual = 0x6;
constexpr u8 GreaterThan = 0x6;
constexpr u8 True = 0x7;
}  // namespace CmpInt

// Used to generate ternary logic truth-tables for vpternlog
// Use these to directly refer to terms and perform binary operations upon them
// and the resulting value will be the ternary lookup table
// ex:
//  (Tern::a | ~Tern::b) & Tern::c
//      = 0b10100010
//      = 0xa2
//  vpternlog a, b, c, 0xa2
//
//  ~(Tern::a ^ Tern::b) & Tern::c
//      = 0b10000010
//      = 0x82
//  vpternlog a, b, c, 0x82
namespace Tern {
constexpr u8 a = 0b11110000;
constexpr u8 b = 0b11001100;
constexpr u8 c = 0b10101010;
}  // namespace Tern

}  // namespace Dynarmic::Backend::X64
