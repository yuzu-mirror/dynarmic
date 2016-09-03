/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "frontend/ir/basic_block.h"
#include "ir_opt/passes.h"

namespace Dynarmic {
namespace Optimization {

void DeadCodeElimination(IR::Block& block) {
    // We iterate over the instructions in reverse order.
    // This is because removing an instruction reduces the number of uses for earlier instructions.

    if (block.empty()) {
        return;
    }

    auto iter = block.end();
    do {
        --iter;
        if (!iter->HasUses() && !iter->MayHaveSideEffects()) {
            iter->Invalidate();
            iter = block.Instructions().erase(iter);
        }
    } while (iter != block.begin());
}

} // namespace Optimization
} // namespace Dynarmic
