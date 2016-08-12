/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <map>
#include <vector>

#include "frontend/ir/opcodes.h"

namespace Dynarmic {
namespace IR {

// Opcode information

namespace OpcodeInfo {

using T = Dynarmic::IR::Type;

struct Meta {
    const char* name;
    Type type;
    std::vector<Type> arg_types;
};

static const std::map<Opcode, Meta> opcode_info {{
#define OPCODE(name, type, ...) { Opcode::name, { #name, type, { __VA_ARGS__ } } },
#include "opcodes.inc"
#undef OPCODE
                                                 }};

} // namespace OpcodeInfo

Type GetTypeOf(Opcode op) {
    return OpcodeInfo::opcode_info.at(op).type;
}

size_t GetNumArgsOf(Opcode op) {
    return OpcodeInfo::opcode_info.at(op).arg_types.size();
}

Type GetArgTypeOf(Opcode op, size_t arg_index) {
    return OpcodeInfo::opcode_info.at(op).arg_types.at(arg_index);
}

const char* GetNameOf(Opcode op) {
    return OpcodeInfo::opcode_info.at(op).name;
}

} // namespace IR
} // namespace Dynarmic
