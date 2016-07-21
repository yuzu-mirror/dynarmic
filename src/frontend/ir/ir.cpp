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

void Value::ReplaceUsesWith(ValuePtr replacement) {
    while (!uses.empty()) {
        auto use = uses.front();
        use.use_owner.lock()->ReplaceUseOfXWithY(use.value.lock(), replacement);
    }
}

std::vector<ValuePtr> Value::GetUses() const {
    std::vector<ValuePtr> result(uses.size());
    std::transform(uses.begin(), uses.end(), result.begin(), [](const auto& use){ return use.use_owner.lock(); });
    return result;
}

void Value::AddUse(ValuePtr owner) {
    // There can be multiple uses from the same owner.
    uses.push_back({ shared_from_this(), owner });
}

void Value::RemoveUse(ValuePtr owner) {
    // Remove only one use.
    auto iter = std::find_if(uses.begin(), uses.end(), [&owner](auto use) { return use.use_owner.lock() == owner; });
    ASSERT_MSG(iter != uses.end(), "RemoveUse without associated AddUse. Bug in use management code.");
    uses.erase(iter);
}

void Value::ReplaceUseOfXWithY(ValuePtr x, ValuePtr y) {
    // This should never be called. Use management is incorrect if this is ever called.
    ASSERT_MSG(false, "This Value type doesn't use any values. Bug in use management code.");
}

void Value::AssertValid() {
    ASSERT(std::all_of(uses.begin(), uses.end(), [](const auto& use) { return !use.use_owner.expired(); }));
}

// Inst class member definitions

Inst::Inst(Opcode op_) : Value(op_) {
    args.resize(GetNumArgsOf(op));
}

void Inst::SetArg(size_t index, ValuePtr value) {
    auto this_ = shared_from_this();

    if (auto prev_value = args.at(index).lock()) {
        prev_value->RemoveUse(this_);
    }

    ASSERT(value->GetType() == GetArgTypeOf(op, index));
    args.at(index) = value;

    value->AddUse(this_);
}

ValuePtr Inst::GetArg(size_t index) const {
    ASSERT_MSG(!args.at(index).expired(), "This should never happen. All Values should be owned by a MicroBlock.");
    return args.at(index).lock();
}

void Inst::Invalidate() {
    AssertValid();
    ASSERT(!HasUses());

    auto this_ = shared_from_this();
    for (auto& arg : args) {
        arg.lock()->RemoveUse(this_);
    }
}

void Inst::AssertValid() {
    ASSERT(std::all_of(args.begin(), args.end(), [](const auto& arg) { return !arg.expired(); }));
    Value::AssertValid();
}

void Inst::ReplaceUseOfXWithY(ValuePtr x, ValuePtr y) {
    bool has_use = false;
    auto this_ = shared_from_this();

    // Note that there may be multiple uses of x.
    for (auto& arg : args) {
        if (arg.lock() == x) {
            arg = y;
            has_use = true;
            x->RemoveUse(this_);
            y->AddUse(this_);
        }
    }

    ASSERT_MSG(has_use, "This Inst doesn't have x. Bug in use management code.");
}

std::string DumpBlock(const IR::Block& block) {
    std::string ret;

    const auto loc_to_string = [](Arm::LocationDescriptor loc) -> std::string {
        return Common::StringFromFormat("{%u,%s,%s}",
                                        loc.arm_pc,
                                        loc.TFlag ? "T" : "!T",
                                        loc.EFlag ? "E" : "!E");
    };

    ret += Common::StringFromFormat("Block: location=%s\n", loc_to_string(block.location).c_str());
    ret += Common::StringFromFormat("cycles=%zu", block.cycle_count);
    ret += Common::StringFromFormat(", entry_cond=%s", Arm::CondToString(block.cond, true));
    if (block.cond != Arm::Cond::AL) {
        ret += Common::StringFromFormat(", cond_fail=%s", loc_to_string(block.cond_failed.get()).c_str());
    }
    ret += "\n";

    std::map<IR::Value*, size_t> value_to_index;
    size_t index = 0;

    const auto arg_to_string = [&value_to_index](IR::ValuePtr arg) -> std::string {
        if (!arg) {
            return "<null>";
        }
        switch (arg->GetOpcode()) {
            case Opcode::ImmU1: {
                auto inst = reinterpret_cast<ImmU1*>(arg.get());
                return Common::StringFromFormat("#%s", inst->value ? "1" : "0");
            }
            case Opcode::ImmU8: {
                auto inst = reinterpret_cast<ImmU8*>(arg.get());
                return Common::StringFromFormat("#%u", inst->value);
            }
            case Opcode::ImmU32: {
                auto inst = reinterpret_cast<ImmU32*>(arg.get());
                return Common::StringFromFormat("#%#x", inst->value);
            }
            case Opcode::ImmRegRef: {
                auto inst = reinterpret_cast<ImmRegRef*>(arg.get());
                return Arm::RegToString(inst->value);
            }
            default: {
                return Common::StringFromFormat("%%%zu", value_to_index.at(arg.get()));
            }
        }
    };

    for (const auto& inst_ptr : block.instructions) {
        const Opcode op = inst_ptr->GetOpcode();
        switch (op) {
            case Opcode::ImmU1:
            case Opcode::ImmU8:
            case Opcode::ImmU32:
            case Opcode::ImmRegRef:
                break;
            default: {
                if (GetTypeOf(op) != Type::Void) {
                    ret += Common::StringFromFormat("%%%-5zu = ", index);
                } else {
                    ret += "         "; // '%00000 = ' -> 1 + 5 + 3 = 9 spaces
                }

                ret += GetNameOf(op);

                const size_t arg_count = GetNumArgsOf(op);
                const auto inst = reinterpret_cast<Inst*>(inst_ptr.get());
                for (size_t arg_index = 0; arg_index < arg_count; arg_index++) {
                    ret += arg_index != 0 ? ", " : " ";
                    ret += arg_to_string(inst->GetArg(arg_index));
                }

                ret += "\n";
                value_to_index[inst_ptr.get()] = index++;
                break;
            }
        }
    }

    return ret;
}

} // namespace IR
} // namespace Dynarmic
