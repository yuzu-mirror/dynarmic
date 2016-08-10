/* This file is part of the dynarmic project.
 * Copyright (c) 2032 MerryMage
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
struct VFP2Matcher {
    using CallRetT = typename mp::MemFnInfo<decltype(&Visitor::vfp2_VADD)>::return_type;

    VFP2Matcher(const char* const name, u32 mask, u32 expect, std::function<CallRetT(Visitor&, u32)> fn)
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
boost::optional<const VFP2Matcher<V>&> DecodeVFP2(u32 instruction) {
    const static std::vector<VFP2Matcher<V>> table = {

#define INST(fn, name, bitstring) detail::detail<VFP2Matcher, u32, 32>::GetMatcher<decltype(fn)>(fn, name, bitstring)

    // cccc1110________----101-__-0----

    // Floating-point three-register data processing instructions
    INST(&V::vfp2_VMLA,           "VMLA",                    "cccc11100D00nnnndddd101zN0M0mmmm"),
    INST(&V::vfp2_VMLS,           "VMLS",                    "cccc11100D00nnnndddd101zN1M0mmmm"),
    INST(&V::vfp2_VNMLS,          "VNMLS",                   "cccc11100D01nnnndddd101zN0M0mmmm"),
    INST(&V::vfp2_VNMLA,          "VNMLA",                   "cccc11100D01nnnndddd101zN1M0mmmm"),
    INST(&V::vfp2_VMUL,           "VMUL",                    "cccc11100D10nnnndddd101zN0M0mmmm"),
    INST(&V::vfp2_VNMUL,          "VNMUL",                   "cccc11100D10nnnndddd101zN1M0mmmm"),
    INST(&V::vfp2_VADD,           "VADD",                    "cccc11100D11nnnndddd101zN0M0mmmm"),
    INST(&V::vfp2_VSUB,           "VSUB",                    "cccc11100D11nnnndddd101zN1M0mmmm"),
    INST(&V::vfp2_VDIV,           "VDIV",                    "cccc11101D00nnnndddd101zN0M0mmmm"),

    // Floating-point move instructions
    INST(&V::vfp2_VMOV_u32_f64,   "VMOV (core to f64)",      "cccc11100000ddddtttt1011D0010000"),
    INST(&V::vfp2_VMOV_f64_u32,   "VMOV (f64 to core)",      "cccc11100001nnnntttt1011N0010000"),
    INST(&V::vfp2_VMOV_u32_f32,   "VMOV (core to f32)",      "cccc11100000nnnntttt1010N0010000"),
    INST(&V::vfp2_VMOV_f32_u32,   "VMOV (f32 to core)",      "cccc11100001nnnntttt1010N0010000"),
    INST(&V::vfp2_VMOV_2u32_2f32, "VMOV (2xcore to 2xf32)",  "cccc11000100uuuutttt101000M1mmmm"),
    INST(&V::vfp2_VMOV_2f32_2u32, "VMOV (2xf32 to 2xcore)",  "cccc11000101uuuutttt101000M1mmmm"),
    INST(&V::vfp2_VMOV_2u32_f64,  "VMOV (2xcore to f64)",    "cccc11000100uuuutttt101100M1mmmm"),
    INST(&V::vfp2_VMOV_f64_2u32,  "VMOV (f64 to 2xcore)",    "cccc11000101uuuutttt101100M1mmmm"),
    INST(&V::vfp2_VMOV_reg,       "VMOV (reg)",              "cccc11101D110000dddd101z01M0mmmm"),

    // Floating-point other instructions
    INST(&V::vfp2_VABS,           "VABS",                    "cccc11101D110000dddd101z11M0mmmm"),
    INST(&V::vfp2_VNEG,           "VNEG",                    "cccc11101D110001dddd101z01M0mmmm"),
    INST(&V::vfp2_VSQRT,          "VSQRT",                   "cccc11101D110001dddd101z11M0mmmm"),
    // VCMP
    // VCMPE
    // VCVT
    // VCVTR

    // Extension register load-store instructions
    // VSTR
    // VSTM
    // VSTMDB
    // VPUSH
    INST(&V::vfp2_VLDR,           "VLDR",                    "cccc1101UD01nnnndddd101zvvvvvvvv"),
    INST(&V::vfp2_VSTR,           "VSTR",                    "cccc1101UD00nnnndddd101zvvvvvvvv"),
    // VLDM
    // VLDMDB
    // VPOP

#undef INST

    };

    if ((instruction & 0xF0000000) == 0xF0000000)
        return boost::none; // Don't try matching any unconditional instructions.

    const auto matches_instruction = [instruction](const auto& matcher){ return matcher.Matches(instruction); };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? boost::make_optional<const VFP2Matcher<V>&>(*iter) : boost::none;
}

} // namespace Arm
} // namespace Dynarmic
