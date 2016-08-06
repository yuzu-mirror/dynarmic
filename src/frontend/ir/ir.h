/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <list>
#include <memory>
#include <vector>

#include <boost/intrusive/list.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/memory_pool.h"
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
    Void      = 1 << 0,
    RegRef    = 1 << 1,
    ExtRegRef = 1 << 2,
    Opaque    = 1 << 3,
    U1        = 1 << 4,
    U8        = 1 << 5,
    U16       = 1 << 6,
    U32       = 1 << 7,
    U64       = 1 << 8,
    F32       = 1 << 9,
    F64       = 1 << 10,
};

Type GetTypeOf(Opcode op);
size_t GetNumArgsOf(Opcode op);
Type GetArgTypeOf(Opcode op, size_t arg_index);
const char* GetNameOf(Opcode op);

// Type declarations

/**
 * A representation of a microinstruction. A single ARM/Thumb instruction may be
 * converted into zero or more microinstructions.
 */

struct Value;
class Inst;

struct Value final {
public:
    Value() : type(Type::Void) {}

    explicit Value(Inst* value) : type(Type::Opaque) {
        inner.inst = value;
    }

    explicit Value(Arm::Reg value) : type(Type::RegRef) {
        inner.imm_regref = value;
    }

    explicit Value(Arm::ExtReg value) : type(Type::ExtRegRef) {
        inner.imm_extregref = value;
    }

    explicit Value(bool value) : type(Type::U1) {
        inner.imm_u1 = value;
    }

    explicit Value(u8 value) : type(Type::U8) {
        inner.imm_u8 = value;
    }

    explicit Value(u32 value) : type(Type::U32) {
        inner.imm_u32 = value;
    }

    bool IsEmpty() const;
    bool IsImmediate() const;
    Type GetType() const;

    Inst* GetInst() const;
    Arm::Reg GetRegRef() const;
    Arm::ExtReg GetExtRegRef() const;
    bool GetU1() const;
    u8 GetU8() const;
    u32 GetU32() const;

private:
    Type type;

    union {
        Inst* inst; // type == Type::Opaque
        Arm::Reg imm_regref;
        Arm::ExtReg imm_extregref;
        bool imm_u1;
        u8 imm_u8;
        u32 imm_u32;
    } inner;
};

using InstListLinkMode = boost::intrusive::link_mode<boost::intrusive::normal_link>;
class Inst final : public boost::intrusive::list_base_hook<InstListLinkMode> {
public:
    Inst(Opcode op) : op(op) {}

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

namespace Term {

struct Invalid {};

/**
 * This terminal instruction calls the interpreter, starting at `next`.
 * The interpreter must interpret at least 1 instruction but may choose to interpret more.
 */
struct Interpret {
    explicit Interpret(const Arm::LocationDescriptor& next_) : next(next_) {}
    Arm::LocationDescriptor next; ///< Location at which interpretation starts.
};

/**
 * This terminal instruction returns control to the dispatcher.
 * The dispatcher will use the value in R15 to determine what comes next.
 */
struct ReturnToDispatch {};

/**
 * This terminal instruction jumps to the basic block described by `next` if we have enough
 * cycles remaining. If we do not have enough cycles remaining, we return to the
 * dispatcher, which will return control to the host.
 */
struct LinkBlock {
    explicit LinkBlock(const Arm::LocationDescriptor& next_) : next(next_) {}
    Arm::LocationDescriptor next; ///< Location descriptor for next block.
};

/**
 * This terminal instruction jumps to the basic block described by `next` unconditionally.
 * This is an optimization and MUST only be emitted when this is guaranteed not to result
 * in hanging, even in the face of other optimizations. (In practice, this means that only
 * forward jumps to short-ish blocks would use this instruction.)
 * A backend that doesn't support this optimization may choose to implement this exactly
 * as LinkBlock.
 */
struct LinkBlockFast {
    explicit LinkBlockFast(const Arm::LocationDescriptor& next_) : next(next_) {}
    Arm::LocationDescriptor next; ///< Location descriptor for next block.
};

/**
 * This terminal instruction checks the top of the Return Stack Buffer against R15.
 * If RSB lookup fails, control is returned to the dispatcher.
 * This is an optimization for faster function calls. A backend that doesn't support
 * this optimization or doesn't have a RSB may choose to implement this exactly as
 * ReturnToDispatch.
 */
struct PopRSBHint {};

struct If;
/// A Terminal is the terminal instruction in a MicroBlock.
using Terminal = boost::variant<
        Invalid,
        Interpret,
        ReturnToDispatch,
        LinkBlock,
        LinkBlockFast,
        PopRSBHint,
        boost::recursive_wrapper<If>
>;

/**
 * This terminal instruction conditionally executes one terminal or another depending
 * on the run-time state of the ARM flags.
 */
struct If {
    If(Arm::Cond if_, Terminal then_, Terminal else_) : if_(if_), then_(then_), else_(else_) {}
    Arm::Cond if_;
    Terminal then_;
    Terminal else_;
};

} // namespace Term

using Term::Terminal;

/**
 * A basic block. It consists of zero or more instructions followed by exactly one terminal.
 * Note that this is a linear IR and not a pure tree-based IR: i.e.: there is an ordering to
 * the microinstructions. This only matters before chaining is done in order to correctly
 * order memory accesses.
 */
class Block final {
public:
    explicit Block(const Arm::LocationDescriptor& location) : location(location) {}

    /// Description of the starting location of this block
    Arm::LocationDescriptor location;
    /// Conditional to pass in order to execute this block
    Arm::Cond cond = Arm::Cond::AL;
    /// Block to execute next if `cond` did not pass.
    boost::optional<Arm::LocationDescriptor> cond_failed = {};

    /// List of instructions in this block.
    boost::intrusive::list<Inst, InstListLinkMode> instructions;
    /// Memory pool for instruction list
    std::unique_ptr<Common::Pool> instruction_alloc_pool = std::make_unique<Common::Pool>(sizeof(Inst), 4096);
    /// Terminal instruction of this block.
    Terminal terminal = Term::Invalid{};

    /// Number of cycles this block takes to execute.
    size_t cycle_count = 0;
};

/// Returns a string representation of the contents of block. Intended for debugging.
std::string DumpBlock(const IR::Block& block);

} // namespace IR
} // namespace Dynarmic
