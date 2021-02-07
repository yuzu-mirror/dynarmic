/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <algorithm>

#include <dynarmic/A32/config.h>

#include "common/assert.h"
#include "common/common_types.h"
#include "frontend/A32/ir_emitter.h"
#include "frontend/A32/translate/conditional_state.h"
#include "frontend/ir/cond.h"

namespace Dynarmic::A32 {

bool CondCanContinue(ConditionalState cond_state, const A32::IREmitter& ir) {
    ASSERT_MSG(cond_state != ConditionalState::Break, "Should never happen.");

    if (cond_state == ConditionalState::None)
        return true;

    // TODO: This is more conservative than necessary.
    return std::all_of(ir.block.begin(), ir.block.end(), [](const IR::Inst& inst) { return !inst.WritesToCPSR(); });
}

bool IsConditionPassed(IR::Cond cond, ConditionalState& cond_state, A32::IREmitter& ir, int instruction_size) {
    ASSERT_MSG(cond_state != ConditionalState::Break,
               "This should never happen. We requested a break but that wasn't honored.");

    if (cond == IR::Cond::NV) {
        // NV conditional is obsolete
        ir.ExceptionRaised(Exception::UnpredictableInstruction);
        return false;
    }

    if (cond_state == ConditionalState::Translating) {
        if (ir.block.ConditionFailedLocation() != ir.current_location || cond == IR::Cond::AL) {
            cond_state = ConditionalState::Trailing;
        } else {
            if (cond == ir.block.GetCondition()) {
                ir.block.SetConditionFailedLocation(ir.current_location.AdvancePC(instruction_size).AdvanceIT());
                ir.block.ConditionFailedCycleCount()++;
                return true;
            }

            // cond has changed, abort
            cond_state = ConditionalState::Break;
            ir.SetTerm(IR::Term::LinkBlockFast{ir.current_location});
            return false;
        }
    }

    if (cond == IR::Cond::AL) {
        // Everything is fine with the world
        return true;
    }

    // non-AL cond

    if (!ir.block.empty()) {
        // We've already emitted instructions. Quit for now, we'll make a new block here later.
        cond_state = ConditionalState::Break;
        ir.SetTerm(IR::Term::LinkBlockFast{ir.current_location});
        return false;
    }

    // We've not emitted instructions yet.
    // We'll emit one instruction, and set the block-entry conditional appropriately.

    cond_state = ConditionalState::Translating;
    ir.block.SetCondition(cond);
    ir.block.SetConditionFailedLocation(ir.current_location.AdvancePC(instruction_size).AdvanceIT());
    ir.block.ConditionFailedCycleCount() = ir.block.CycleCount() + 1;
    return true;
}

} // namespace Dynarmic::A32
