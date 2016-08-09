/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <algorithm>
#include <map>

#include "common/assert.h"
#include "common/string_util.h"
#include "frontend/ir/ir.h"

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

// Value class member definitions

bool Value::IsImmediate() const {
    if (type == Type::Opaque)
        return inner.inst->GetOpcode() == Opcode::Identity ? inner.inst->GetArg(0).IsImmediate() : false;
    return true;
}

bool Value::IsEmpty() const {
    return type == Type::Void;
}

Type Value::GetType() const {
    if (type == Type::Opaque) {
        if (inner.inst->GetOpcode() == Opcode::Identity) {
            return inner.inst->GetArg(0).GetType();
        } else {
            return inner.inst->GetType();
        }
    }
    return type;
}

Arm::Reg Value::GetRegRef() const {
    DEBUG_ASSERT(type == Type::RegRef);
    return inner.imm_regref;
}

Arm::ExtReg Value::GetExtRegRef() const {
    DEBUG_ASSERT(type == Type::ExtRegRef);
    return inner.imm_extregref;
}

Inst* Value::GetInst() const {
    DEBUG_ASSERT(type == Type::Opaque);
    return inner.inst;
}

bool Value::GetU1() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU1();
    DEBUG_ASSERT(type == Type::U1);
    return inner.imm_u1;
}

u8 Value::GetU8() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU8();
    DEBUG_ASSERT(type == Type::U8);
    return inner.imm_u8;
}

u32 Value::GetU32() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU32();
    DEBUG_ASSERT(type == Type::U32);
    return inner.imm_u32;
}

// Inst class member definitions

Value Inst::GetArg(size_t index) const {
    DEBUG_ASSERT(index < GetNumArgsOf(op));
    DEBUG_ASSERT(!args[index].IsEmpty());

    return args[index];
}

void Inst::SetArg(size_t index, Value value) {
    DEBUG_ASSERT(index < GetNumArgsOf(op));
    DEBUG_ASSERT(value.GetType() == GetArgTypeOf(op, index) || Type::Opaque == GetArgTypeOf(op, index));

    if (!args[index].IsImmediate()) {
        UndoUse(args[index]);
    }
    if (!value.IsImmediate()) {
        Use(value);
    }

    args[index] = value;
}

void Inst::Invalidate() {
    for (auto& value : args) {
        if (!value.IsImmediate()) {
            UndoUse(value);
        }
    }
}

void Inst::ReplaceUsesWith(Value& replacement) {
    Invalidate();

    op = Opcode::Identity;

    if (!replacement.IsImmediate()) {
        Use(replacement);
    }

    args[0] = replacement;
}

void Inst::Use(Value& value) {
    value.GetInst()->use_count++;

    switch (op){
        case Opcode::GetCarryFromOp:
            value.GetInst()->carry_inst = this;
            break;
        case Opcode::GetOverflowFromOp:
            value.GetInst()->overflow_inst = this;
            break;
        default:
            break;
    }
}

void Inst::UndoUse(Value& value) {
    value.GetInst()->use_count--;

    switch (op){
        case Opcode::GetCarryFromOp:
            value.GetInst()->carry_inst = nullptr;
            break;
        case Opcode::GetOverflowFromOp:
            value.GetInst()->overflow_inst = nullptr;
            break;
        default:
            break;
    }
}

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
            ret += arg_index != 0 ? ", " : " ";
            ret += arg_to_string(inst->GetArg(arg_index));
        }

        ret += "\n";
        inst_to_index[&*inst] = index++;
    }

    return ret;
}

} // namespace IR
} // namespace Dynarmic
