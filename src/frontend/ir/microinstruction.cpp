/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/ir/microinstruction.h"

namespace Dynarmic {
namespace IR {

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

} // namespace IR
} // namespace Dynarmic
