/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <functional>
#include <vector>

#include <boost/optional.hpp>

#include "common/common_types.h"
#include "frontend/decoder/decoder_detail.h"

namespace Dynarmic {
namespace Arm {

template <typename Visitor>
struct Thumb32Matcher {
    using CallRetT = typename mp::MemFnInfo<decltype(&Visitor::thumb32_UDF), &Visitor::thumb32_UDF>::return_type;

    Thumb32Matcher(const char* const name, u32 mask, u32 expect, std::function<CallRetT(Visitor&, u32)> fn)
            : name(name), mask(mask), expect(expect), fn(fn) {}

    /// Gets the name of this type of instruction.
    const char* GetName() const {
        return name;
    }

    /**
     * Tests to see if the instruction is this type of instruction.
     * @param instruction The instruction to test
     * @returns true if the instruction is
     */
    bool Matches(u32 instruction) const {
        return (instruction & mask) == expect;
    }

    /**
     * Calls the corresponding instruction handler on visitor for this type of instruction.
     * @param v The visitor to use
     * @param instruction The instruction to decode.
     */
    CallRetT call(Visitor& v, u32 instruction) const {
        assert(Matches(instruction));
        return fn(v, instruction);
    }

private:
    const char* name;
    u32 mask, expect;
    std::function<CallRetT(Visitor&, u32)> fn;
};

template<typename V>
boost::optional<const Thumb32Matcher<V>&> DecodeThumb32(u32 instruction) {
    const static std::vector<Thumb32Matcher<V>> table = {

#define INST(fn, name, bitstring) detail::detail<Thumb32Matcher, u32, 32>::GetMatcher<decltype(fn), fn>(name, bitstring)

        // Branch instructions
        INST(&V::thumb32_BL_imm,         "BL (imm)",                 "11110vvvvvvvvvvv11111vvvvvvvvvvv"), // v4T
        INST(&V::thumb32_BLX_imm,        "BLX (imm)",                "11110vvvvvvvvvvv11101vvvvvvvvvvv"), // v5T

        // Misc instructions
        INST(&V::thumb32_UDF,            "UDF",                      "111101111111----1010------------"), // v6T2

#undef INST

    };

    const auto matches_instruction = [instruction](const auto& matcher){ return matcher.Matches(instruction); };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? boost::make_optional<const Thumb32Matcher<V>&>(*iter) : boost::none;
}

} // namespace Arm
} // namespace Dynarmic
