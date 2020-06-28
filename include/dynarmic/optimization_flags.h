/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstdint>

namespace Dynarmic {

enum class OptimizationFlag : std::uint32_t {
    BlockLinking        = 0x01,
    ReturnStackBuffer   = 0x02,
    FastDispatch        = 0x04,
    GetSetElimination   = 0x08,
    ConstProp           = 0x10,
    MiscIROpt           = 0x20,
};

constexpr OptimizationFlag no_optimizations = static_cast<OptimizationFlag>(0);
constexpr OptimizationFlag all_optimizations = static_cast<OptimizationFlag>(~std::uint32_t(0));

constexpr OptimizationFlag operator~(OptimizationFlag f) {
    return static_cast<OptimizationFlag>(~static_cast<std::uint32_t>(f));
}

constexpr OptimizationFlag operator|(OptimizationFlag f1, OptimizationFlag f2) {
    return static_cast<OptimizationFlag>(static_cast<std::uint32_t>(f1) | static_cast<std::uint32_t>(f2));
}

constexpr OptimizationFlag operator&(OptimizationFlag f1, OptimizationFlag f2) {
    return static_cast<OptimizationFlag>(static_cast<std::uint32_t>(f1) & static_cast<std::uint32_t>(f2));
}

constexpr OptimizationFlag operator|=(OptimizationFlag& result, OptimizationFlag f) {
    return result = (result | f);
}

constexpr OptimizationFlag operator&=(OptimizationFlag& result, OptimizationFlag f) {
    return result = (result & f);
}

constexpr bool operator!(OptimizationFlag f) {
    return f == no_optimizations;
}

} // namespace Dynarmic
