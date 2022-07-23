/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

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

}  // namespace Dynarmic::Backend::Arm64
