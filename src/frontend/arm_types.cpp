/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/string_util.h"
#include "frontend/arm_types.h"

namespace Dynarmic {
namespace Arm {

const char* CondToString(Cond cond, bool explicit_al) {
    constexpr std::array<const char*, 15> cond_strs = {
            "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "al"
    };
    return (!explicit_al && cond == Cond::AL) ? "" : cond_strs.at(static_cast<size_t>(cond));
}

const char* RegToString(Reg reg) {
    constexpr std::array<const char*, 16> reg_strs = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
    };
    return reg_strs.at(static_cast<size_t>(reg));
}

std::string RegListToString(RegList reg_list) {
    std::string ret = "";
    bool first_reg = true;
    for (size_t i = 0; i < 16; i++) {
        if (Common::Bit(i, reg_list)) {
            if (!first_reg)
                ret += ", ";
            ret += RegToString(static_cast<Reg>(i));
            first_reg = false;
        }
    }
    return ret;
}

} // namespace Arm
} // namespace Dynarmic
