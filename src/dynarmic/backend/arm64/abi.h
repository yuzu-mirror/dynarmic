/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <initializer_list>

#include <mcl/stdint.hpp>
#include <oaknut/oaknut.hpp>

namespace Dynarmic::Backend::Arm64 {

constexpr oaknut::XReg Xstate{28};
constexpr oaknut::XReg Xhalt{27};

constexpr oaknut::XReg Xscratch0{16}, Xscratch1{17};
constexpr oaknut::WReg Wscratch0{16}, Wscratch1{17};

template<size_t bitsize>
constexpr auto Rscratch0() {
    if constexpr (bitsize == 32) {
        return Wscratch0;
    } else if constexpr (bitsize == 64) {
        return Xscratch0;
    } else {
        static_assert(bitsize == 32 || bitsize == 64);
    }
}

template<size_t bitsize>
constexpr auto Rscratch1() {
    if constexpr (bitsize == 32) {
        return Wscratch1;
    } else if constexpr (bitsize == 64) {
        return Xscratch1;
    } else {
        static_assert(bitsize == 32 || bitsize == 64);
    }
}

constexpr std::initializer_list<int> GPR_ORDER{19, 20, 21, 22, 23, 24, 25, 26, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8};
constexpr std::initializer_list<int> FPR_ORDER{8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

using RegisterList = u64;

constexpr RegisterList ABI_CALLEE_SAVE = 0x0000ff00'3ff80000;
constexpr RegisterList ABI_CALLER_SAVE = 0xffffffff'4000ffff;

void ABI_PushRegisters(oaknut::CodeGenerator& code, RegisterList rl, size_t stack_space);
void ABI_PopRegisters(oaknut::CodeGenerator& code, RegisterList rl, size_t stack_space);

}  // namespace Dynarmic::Backend::Arm64
