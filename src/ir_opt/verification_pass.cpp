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

void VerificationPass(const IR::Block& block) {
    for (const auto& inst : block.instructions) {
        inst->AssertValid();
    }
}

} // namespace Optimization
} // namespace Dynarmic
