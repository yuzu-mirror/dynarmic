/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/ir/ir.h"
#include "ir_opt/passes.h"

namespace Dynarmic {
namespace Optimization {

void DeadCodeElimination(IR::Block& block) {
    const auto is_side_effect_free = [](IR::Opcode op) -> bool {
        return IR::GetTypeOf(op) != IR::Type::Void;
    };

    // We iterate over the instructions in reverse order.
    // This is because removing an instruction reduces the number of uses for earlier instructions.

    if (block.instructions.IsEmpty()) {
        return;
    }

    auto iter = block.instructions.end();
    do {
        --iter;
        if (!iter->HasUses() && is_side_effect_free(iter->GetOpcode())) {
            iter->Invalidate();
            iter = block.instructions.erase(iter);
        }
    } while (iter != block.instructions.begin());
}

} // namespace Optimization
} // namespace Dynarmic
