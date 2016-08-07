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
    // VMLA
    // VMLS
    // VNMLA
    // VNMLS
    // VNMUL
    // VMUL
    INST(&V::vfp2_VADD,       "VADD",                "cccc11100D11nnnndddd101zN0M0mmmm"),
    INST(&V::vfp2_VSUB,       "VSUB",                "cccc11100D11nnnndddd101zN1M0mmmm"),
    // VDIV

    // Floating-point other instructions
    // VMOV_imm
    // VMOV_reg
    INST(&V::vfp2_VABS,       "VABS",                "cccc11101D110000dddd101z11M0mmmm"),
    // VNEG
    // VSQRT
    // VCMP
    // VCMPE
    // VCVT
    // VCVTR

    // Extension register load-store instructions
    // VSTR
    // VSTM
    // VSTMDB
    // VPUSH
    // VLDR
    // VLDM
    // VLDMDB
    // VPOP

#undef INST

    };

    const auto matches_instruction = [instruction](const auto& matcher){ return matcher.Matches(instruction); };

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? boost::make_optional<const VFP2Matcher<V>&>(*iter) : boost::none;
}

} // namespace Arm
} // namespace Dynarmic
