/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <algorithm>
#include <map>

#include "common/assert.h"
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
    for (const auto& use : uses) {
        use.use_owner.lock()->ReplaceUseOfXWithY(use.value.lock(), replacement);
    }
    assert(uses.empty());
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

void Inst::AssertValid() {
    ASSERT(std::all_of(args.begin(), args.end(), [](const auto& arg) { return !arg.expired(); }));
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

} // namespace IR
} // namespace Dynarmic
