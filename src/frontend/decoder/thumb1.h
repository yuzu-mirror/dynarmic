/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <functional>
#include <tuple>

#include <boost/optional.hpp>

#include "common/common_types.h"
#include "frontend/decoder/decoder_detail.h"

namespace Dynarmic {
namespace Arm {

template <typename Visitor>
struct Thumb1Matcher {
    using CallRetT = typename mp::MemFnInfo<decltype(&Visitor::thumb1_UDF), &Visitor::thumb1_UDF>::return_type;

    Thumb1Matcher(const char* const name, u16 mask, u16 expect, std::function<CallRetT(Visitor&, u16)> fn)
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
    bool Matches(u16 instruction) const {
        return (instruction & mask) == expect;
    }

    /**
     * Calls the corresponding instruction handler on visitor for this type of instruction.
     * @param v The visitor to use
     * @param instruction The instruction to decode.
     */
    CallRetT call(Visitor& v, u16 instruction) const {
        assert(Matches(instruction));
        return fn(v, instruction);
    }

private:
    const char* name;
    u16 mask, expect;
    std::function<CallRetT(Visitor&, u16)> fn;
};

template <typename V>
static const std::array<Thumb1Matcher<V>, 24> g_thumb1_instruction_table {{

#define INST(fn, name, bitstring) detail::detail<Thumb1Matcher, u16, 16>::GetMatcher<decltype(fn), fn>(name, bitstring)

    // Shift (immediate), add, subtract, move and compare instructions
    { INST(&V::thumb1_LSL_imm,        "LSL (imm)",                "00000vvvvvmmmddd") },
    { INST(&V::thumb1_LSR_imm,        "LSR (imm)",                "00001vvvvvmmmddd") },
    { INST(&V::thumb1_ASR_imm,        "ASR (imm)",                "00010vvvvvmmmddd") },
    { INST(&V::thumb1_ADD_reg_t1,     "ADD (reg, T1)",            "0001100mmmnnnddd") },
    { INST(&V::thumb1_SUB_reg,        "SUB (reg)",                "0001101mmmnnnddd") },
    { INST(&V::thumb1_ADD_imm_t1,     "ADD (imm, T1)",            "0001110vvvnnnddd") },
    { INST(&V::thumb1_SUB_imm_t1,     "SUB (imm, T1)",            "0001111vvvnnnddd") },
    { INST(&V::thumb1_MOV_imm,        "MOV (imm)",                "00100dddvvvvvvvv") },
    { INST(&V::thumb1_CMP_imm,        "CMP (imm)",                "00101nnnvvvvvvvv") },
    { INST(&V::thumb1_ADD_imm_t2,     "ADD (imm, T2)",            "00110dddvvvvvvvv") },
    { INST(&V::thumb1_SUB_imm_t2,     "SUB (imm, T2)",            "00111dddvvvvvvvv") },

     // Data-processing instructions
    { INST(&V::thumb1_AND_reg,        "AND (reg)",                "0100000000mmmddd") },
    { INST(&V::thumb1_EOR_reg,        "EOR (reg)",                "0100000001mmmddd") },
    { INST(&V::thumb1_LSL_reg,        "LSL (reg)",                "0100000010mmmddd") },
    { INST(&V::thumb1_LSR_reg,        "LSR (reg)",                "0100000011mmmddd") },
    { INST(&V::thumb1_ASR_reg,        "ASR (reg)",                "0100000100mmmddd") },
    { INST(&V::thumb1_ADC_reg,        "ADC (reg)",                "0100000101mmmddd") },
    { INST(&V::thumb1_SBC_reg,        "SBC (reg)",                "0100000110mmmddd") },
    { INST(&V::thumb1_ROR_reg,        "ROR (reg)",                "0100000111sssddd") },
    { INST(&V::thumb1_TST_reg,        "TST (reg)",                "0100001000mmmnnn") },
    { INST(&V::thumb1_RSB_imm,        "RSB (imm)",                "0100001001nnnddd") },
    { INST(&V::thumb1_CMP_reg,        "CMP (reg)",                "0100001010mmmnnn") },
    //{ INST(&V::thumb1_CMN_rr,         "CMN (rr)",                 "0100001011mmmnnn") },
    //{ INST(&V::thumb1_ORRS_rr,        "ORRS (rr)",                "0100001100mmmddd") },
    //{ INST(&V::thumb1_MULS_rr,        "MULS (rr)",                "0100001101mmmddd") },
    //{ INST(&V::thumb1_BICS_rr,        "BICS (rr)",                "0100001110mmmddd") },
    //{ INST(&V::thumb1_MVNS_rr,        "MVNS (rr)",                "0100001111mmmddd") },

    // Special data instructions
    { INST(&V::thumb1_ADD_reg_t2,     "ADD (reg, T2)",            "01000100Dmmmmddd") }, // v4T, Low regs: v6T2
    //{ INST(&V::thumb1_CMP_high,       "CMP (high)",               "01000101dmmmmddd") }, // v4T
    //{ INST(&V::thumb1_MOV_high,       "MOV (high)",               "01000110dmmmmddd") }, // v4T, Low regs: v6

    // Store/Load single data item instructions
    //{ INST(&V::thumb1_LDR_lit,        "LDR (literal)",            "01001dddvvvvvvvv") },
    //{ INST(&V::thumb1_STR_rrr,        "STR (rrr)",                "0101000mmmnnnddd") },
    //{ INST(&V::thumb1_STRH_rrr,       "STRH (rrr)",               "0101001mmmnnnddd") },
    //{ INST(&V::thumb1_STRB_rrr,       "STRB (rrr)",               "0101010mmmnnnddd") },
    //{ INST(&V::thumb1_LDRSB_rrr,      "LDRSB (rrr)",              "0101011mmmnnnddd") },
    //{ INST(&V::thumb1_LDR_rrr,        "LDR (rrr)",                "0101100mmmnnnddd") },
    //{ INST(&V::thumb1_LDRH_rrr,       "LDRH (rrr)",               "0101101mmmnnnddd") },
    //{ INST(&V::thumb1_LDRB_rrr,       "LDRB (rrr)",               "0101110mmmnnnddd") },
    //{ INST(&V::thumb1_LDRSH_rrr,      "LDRSH (rrr)",              "0101111mmmnnnddd") },
    //{ INST(&V::thumb1_STRH_rri,       "STRH (rri)",               "10000vvvvvnnnddd") },
    //{ INST(&V::thumb1_LDRH_rri,       "LDRH (rri)",               "10001vvvvvnnnddd") },
    //{ INST(&V::thumb1_STR_sp,         "STR (SP)",                 "10010dddvvvvvvvv") },
    //{ INST(&V::thumb1_LDR_sp,         "LDR (SP)",                 "10011dddvvvvvvvv") },

    // Generate relative address instruction
    //{ INST(&V::thumb1_ADR,            "ADR",                      "10100dddvvvvvvvv") },
    //{ INST(&V::thumb1_ADD_sp,         "ADD (relative to SP)",     "10101dddvvvvvvvv") },

    // Miscellaneous 16-bit instructions
    //{ INST(&V::thumb1_ADD_spsp,       "ADD (imm to SP)",          "101100000vvvvvvv") }, // v4T
    //{ INST(&V::thumb1_SUB_spsp,       "SUB (imm from SP)",        "101100001vvvvvvv") }, // v4T
    //{ INST(&V::thumb1_SXTH,           "SXTH",                     "1011001000mmmddd") }, // v6
    //{ INST(&V::thumb1_SXTB,           "SXTB",                     "1011001001mmmddd") }, // v6
    //{ INST(&V::thumb1_UXTH,           "UXTH",                     "1011001010mmmddd") }, // v6
    //{ INST(&V::thumb1_UXTB,           "UXTB",                     "1011001011mmmddd") }, // v6
    //{ INST(&V::thumb1_PUSH,           "PUSH",                     "1011010rxxxxxxxx") }, // v4T
    //{ INST(&V::thumb1_POP,            "POP",                      "1011110rxxxxxxxx") }, // v4T
    //{ INST(&V::thumb1_SETEND,         "SETEND",                   "101101100101x000") }, // v6
    //{ INST(&V::thumb1_CPS,            "CPS",                      "10110110011m0aif") }, // v6
    //{ INST(&V::thumb1_REV,            "REV",                      "1011101000nnnddd") }, // v6
    //{ INST(&V::thumb1_REV16,          "REV16",                    "1011101001nnnddd") }, // v6
    //{ INST(&V::thumb1_REVSH,          "REVSH",                    "1011101011nnnddd") }, // v6
    //{ INST(&V::thumb1_BKPT,           "BKPT",                     "10111110xxxxxxxx") }, // v5

    // Store/Load multiple registers
    //{ INST(&V::thumb1_STMIA,          "STMIA",                    "11000nnnxxxxxxxx") },
    //{ INST(&V::thumb1_LDMIA,          "LDMIA",                    "11001nnnxxxxxxxx") },

    // Branch instructions
    //{ INST(&V::thumb1_BX,             "BX (reg)",                 "010001110mmmm000") }, // v4T
    //{ INST(&V::thumb1_BLX,            "BLX (reg)",                "010001111mmmm000") }, // v5T
    { INST(&V::thumb1_UDF,            "UDF",                      "11011110--------") },
    //{ INST(&V::thumb1_SWI,            "SWI",                      "11011111xxxxxxxx") },
    //{ INST(&V::thumb1_B_cond,         "B (cond)",                 "1101ccccxxxxxxxx") },
    //{ INST(&V::thumb1_B_imm,          "B (imm)",                  "11100xxxxxxxxxxx") },
    //{ INST(&V::thumb1_BLX_suffix,     "BLX (imm, suffix)",        "11101xxxxxxxxxx0") },
    //{ INST(&V::thumb1_BLX_prefix,     "BL/BLX (imm, prefix)",     "11110xxxxxxxxxxx") },
    //{ INST(&V::thumb1_BL_suffix,      "BL (imm, suffix)",         "11111xxxxxxxxxxx") },

#undef INST

}};

template<typename Visitor>
boost::optional<const Thumb1Matcher<Visitor>&> DecodeThumb16(u16 instruction) {
    const auto& table = g_thumb1_instruction_table<Visitor>;
    auto matches_instruction = [instruction](const auto& matcher){ return matcher.Matches(instruction); };

    assert(std::count_if(table.begin(), table.end(), matches_instruction) <= 1);

    auto iter = std::find_if(table.begin(), table.end(), matches_instruction);
    return iter != table.end() ? boost::make_optional<const Thumb1Matcher<Visitor>&>(*iter) : boost::none;
}

} // namespace Arm
} // namespace Dynarmic
