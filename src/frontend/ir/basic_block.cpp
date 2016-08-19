/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <map>
#include <string>

#include "common/string_util.h"
#include "frontend/ir/basic_block.h"

namespace Dynarmic {
namespace IR {

std::string DumpBlock(const IR::Block& block) {
    std::string ret;

    const auto loc_to_string = [](Arm::LocationDescriptor loc) -> std::string {
        return Common::StringFromFormat("{%u,%s,%s,%u}",
                                        loc.PC(),
                                        loc.TFlag() ? "T" : "!T",
                                        loc.EFlag() ? "E" : "!E",
                                        loc.FPSCR());
    };

    ret += Common::StringFromFormat("Block: location=%s\n", loc_to_string(block.location).c_str());
    ret += Common::StringFromFormat("cycles=%zu", block.cycle_count);
    ret += Common::StringFromFormat(", entry_cond=%s", Arm::CondToString(block.cond, true));
    if (block.cond != Arm::Cond::AL) {
        ret += Common::StringFromFormat(", cond_fail=%s", loc_to_string(block.cond_failed.get()).c_str());
    }
    ret += "\n";

    std::map<const IR::Inst*, size_t> inst_to_index;
    size_t index = 0;

    const auto arg_to_string = [&inst_to_index](const IR::Value& arg) -> std::string {
        if (arg.IsEmpty()) {
            return "<null>";
        } else if (!arg.IsImmediate()) {
            return Common::StringFromFormat("%%%zu", inst_to_index.at(arg.GetInst()));
        }
        switch (arg.GetType()) {
            case Type::U1:
                return Common::StringFromFormat("#%s", arg.GetU1() ? "1" : "0");
            case Type::U8:
                return Common::StringFromFormat("#%u", arg.GetU8());
            case Type::U32:
                return Common::StringFromFormat("#%#x", arg.GetU32());
            case Type::RegRef:
                return Arm::RegToString(arg.GetRegRef());
            case Type::ExtRegRef:
                return Arm::ExtRegToString(arg.GetExtRegRef());
            default:
                return "<unknown immediate type>";
        }
    };

    for (auto inst = block.instructions.begin(); inst != block.instructions.end(); ++inst) {
        const Opcode op = inst->GetOpcode();

        if (GetTypeOf(op) != Type::Void) {
            ret += Common::StringFromFormat("%%%-5zu = ", index);
        } else {
            ret += "         "; // '%00000 = ' -> 1 + 5 + 3 = 9 spaces
        }

        ret += GetNameOf(op);

        const size_t arg_count = GetNumArgsOf(op);
        for (size_t arg_index = 0; arg_index < arg_count; arg_index++) {
            const Value arg = inst->GetArg(arg_index);

            ret += arg_index != 0 ? ", " : " ";
            ret += arg_to_string(arg);

            Type actual_type = arg.GetType();
            Type expected_type = GetArgTypeOf(op, arg_index);
            if (!AreTypesCompatible(actual_type, expected_type)) {
                ret += Common::StringFromFormat("<type error: %s != %s>", GetNameOf(actual_type), GetNameOf(expected_type));
            }
        }

        ret += "\n";
        inst_to_index[&*inst] = index++;
    }

    return ret;
}

} // namespace IR
} // namespace Dynarmic
