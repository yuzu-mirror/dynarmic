/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/iterator_util.h"
#include "ir/basic_block.h"
#include "ir/opt/passes.h"

namespace Dynarmic::Optimization {

void DeadCodeElimination(IR::Block& block) {
    // We iterate over the instructions in reverse order.
    // This is because removing an instruction reduces the number of uses for earlier instructions.
    for (auto& inst : Common::Reverse(block)) {
        if (!inst.HasUses() && !inst.MayHaveSideEffects()) {
            inst.Invalidate();
        }
    }
}

} // namespace Dynarmic
