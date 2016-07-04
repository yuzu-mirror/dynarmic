/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <list>
#include <memory>
#include <vector>

#include <boost/variant.hpp>

#include "common/common_types.h"
#include "frontend/arm_types.h"
#include "frontend/ir/opcodes.h"

namespace Dynarmic {
namespace IR {

// ARM JIT Microinstruction Intermediate Representation
//
// This intermediate representation is an SSA IR. It is designed primarily for analysis,
// though it can be lowered into a reduced form for interpretation. Each IR node (Value)
// is a microinstruction of an idealised ARM CPU. The choice of microinstructions is made
// not based on any existing microarchitecture but on ease of implementation.
//
// A basic block is represented as an IR::Block.

enum class Type {
    Void    = 1 << 0,
    RegRef  = 1 << 1,
    Opaque  = 1 << 2,
    U1      = 1 << 3,
    U8      = 1 << 4,
    U16     = 1 << 5,
    U32     = 1 << 6,
    U64     = 1 << 7,
};

Type GetTypeOf(Opcode op);
size_t GetNumArgsOf(Opcode op);
Type GetArgTypeOf(Opcode op, size_t arg_index);
const char* GetNameOf(Opcode op);

// Type declarations

/// Base class for microinstructions to derive from.

class Value;
using ValuePtr = std::shared_ptr<Value>;
using ValueWeakPtr = std::weak_ptr<Value>;

class Value : public std::enable_shared_from_this<Value> {
public:
    virtual ~Value() = default;

    bool HasUses() const { return !uses.empty(); }
    bool HasOneUse() const { return uses.size() == 1; }
    bool HasManyUses() const { return uses.size() > 1; }

    /// Replace all uses of this Value with `replacement`.
    void ReplaceUsesWith(ValuePtr replacement);

    /// Get the microop this microinstruction represents.
    Opcode GetOpcode() const { return op; }
    /// Get the type this instruction returns.
    Type GetType() const { return GetTypeOf(op); }
    /// Get the number of arguments this instruction has.
    size_t NumArgs() const { return GetNumArgsOf(op); }
    /// Get the number of uses this instruction has.
    size_t NumUses() const { return uses.size(); }

    std::vector<ValuePtr> GetUses() const;

    intptr_t GetTag() const { return tag; }
    void SetTag(intptr_t tag_) { tag = tag_; }

protected:
    friend class Inst;

    Value(Opcode op_) : op(op_) {}

    void AddUse(ValuePtr owner);
    void RemoveUse(ValuePtr owner);
    virtual void ReplaceUseOfXWithY(ValuePtr x, ValuePtr y);

private:
    Opcode op;

    struct Use {
        /// The instruction which is being used.
        ValueWeakPtr value;
        /// The instruction which is using `value`.
        ValueWeakPtr use_owner;
    };
    std::list<Use> uses;

    intptr_t tag;
};

/// Representation of a u8 immediate.
class ImmU8 final : public Value {
public:
    explicit ImmU8(u8 value_) : Value(Opcode::ImmU8), value(value_) {}
    ~ImmU8() override = default;

    const u8 value; ///< Literal value to load
};

/// Representation of a u32 immediate.
class ImmU32 final : public Value {
public:
    explicit ImmU32(u32 value_) : Value(Opcode::ImmU32), value(value_) {}
    ~ImmU32() override = default;

    const u32 value; ///< Literal value to load
};

/// Representation of a GPR reference.
class ImmRegRef final : public Value {
public:
    explicit ImmRegRef(Arm::Reg value_) : Value(Opcode::ImmRegRef), value(value_) {}
    ~ImmRegRef() override = default;

    const Arm::Reg value; ///< Literal value to load
};

/**
 * A representation of a microinstruction. A single ARM/Thumb instruction may be
 * converted into zero or more microinstructions.
 */
class Inst final : public Value {
public:
    explicit Inst(Opcode op);
    ~Inst() override = default;

    /// Set argument number `index` to `value`.
    void SetArg(size_t index, ValuePtr value);
    /// Get argument number `index`.
    ValuePtr GetArg(size_t index) const;

    void AssertValid();
protected:
    void ReplaceUseOfXWithY(ValuePtr x, ValuePtr y) override;

private:
    std::vector<ValueWeakPtr> args;
};

/**
 * A basic block. It consists of zero or more instructions followed by exactly one terminal.
 * Note that this is a linear IR and not a pure tree-based IR: i.e.: there is an ordering to
 * the microinstructions. This only matters before chaining is done in order to correctly
 * order memory accesses.
 */
class Block final {
public:
    explicit Block(const Arm::LocationDescriptor& location) : location(location) {}

    Arm::LocationDescriptor location;
    std::list<ValuePtr> instructions;
};


} // namespace IR
} // namespace Dynarmic
