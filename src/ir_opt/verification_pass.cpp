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
#if 0
    for (const auto& inst : block.instructions) {
        inst->AssertValid();
    }
#endif
}

} // namespace Optimization
} // namespace Dynarmic
