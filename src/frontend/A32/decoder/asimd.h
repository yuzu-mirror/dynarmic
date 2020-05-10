/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <vector>

#include "common/bit_util.h"
#include "common/common_types.h"
#include "frontend/decoder/decoder_detail.h"
#include "frontend/decoder/matcher.h"

namespace Dynarmic::A32 {

template <typename Visitor>
using ASIMDMatcher = Decoder::Matcher<Visitor, u32>;

template <typename V>
std::vector<ASIMDMatcher<V>> GetASIMDDecodeTable() {
    std::vector<ASIMDMatcher<V>> table = {

#define INST(fn, name, bitstring) Decoder::detail::detail<ASIMDMatcher<V>>::GetMatcher(&V::fn, name, bitstring),
#include "asimd.inc"
#undef INST

    };

    // If a matcher has more bits in its mask it is more specific, so it should come first.
    std::stable_sort(table.begin(), table.end(), [](const auto& matcher1, const auto& matcher2) {
        return Common::BitCount(matcher1.GetMask()) > Common::BitCount(matcher2.GetMask());
    });

    return table;
}

template<typename V>
std::optional<std::reference_wrapper<const ASIMDMatcher<V>>> DecodeASIMD(u32 instruction) {
    static const auto table = GetASIMDDecodeTable<V>();

    const auto matches_instruction = [instruction](const auto& matcher) { return matcher.Matches(instruction); };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? std::optional<std::reference_wrapper<const ASIMDMatcher<V>>>(*iter) : std::nullopt;
}

} // namespace Dynarmic::A32
