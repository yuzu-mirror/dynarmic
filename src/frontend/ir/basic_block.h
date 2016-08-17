/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <memory>
#include <string>

#include <boost/optional.hpp>

#include "common/common_types.h"
#include "common/intrusive_list.h"
#include "common/memory_pool.h"
#include "frontend/arm_types.h"
#include "frontend/ir/microinstruction.h"
#include "frontend/ir/terminal.h"

namespace Dynarmic {
namespace IR {

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
    Common::IntrusiveList<Inst> instructions;
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
