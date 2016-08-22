/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>

#include "common/common_types.h"
#include "common/intrusive_list.h"
#include "frontend/ir/opcodes.h"
#include "frontend/ir/value.h"

namespace Dynarmic {
namespace IR {

/**
 * A representation of a microinstruction. A single ARM/Thumb instruction may be
 * converted into zero or more microinstructions.
 */
class Inst final : public Common::IntrusiveListNode<Inst> {
public:
    explicit Inst(Opcode op) : op(op) {}

    bool HasUses() const { return use_count > 0; }

    /// Get the microop this microinstruction represents.
    Opcode GetOpcode() const { return op; }
    /// Get the type this instruction returns.
    Type GetType() const { return GetTypeOf(op); }
    /// Get the number of arguments this instruction has.
    size_t NumArgs() const { return GetNumArgsOf(op); }

    Value GetArg(size_t index) const;
    void SetArg(size_t index, Value value);

    void Invalidate();

    void ReplaceUsesWith(Value& replacement);

    size_t use_count = 0;
    Inst* carry_inst = nullptr;
    Inst* overflow_inst = nullptr;

private:
    void Use(Value& value);
    void UndoUse(Value& value);

    Opcode op;
    std::array<Value, 3> args;
};

} // namespace IR
} // namespace Dynarmic
