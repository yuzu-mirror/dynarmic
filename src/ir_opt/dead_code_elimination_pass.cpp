/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/ir/basic_block.h"
#include "frontend/ir/opcodes.h"
#include "ir_opt/passes.h"

namespace Dynarmic {
namespace Optimization {

void DeadCodeElimination(IR::Block& block) {
    const auto is_side_effect_free = [](IR::Opcode op) -> bool {
        switch (op) {
        case IR::Opcode::Breakpoint:
        case IR::Opcode::SetRegister:
        case IR::Opcode::SetExtendedRegister32:
        case IR::Opcode::SetExtendedRegister64:
        case IR::Opcode::SetCpsr:
        case IR::Opcode::SetNFlag:
        case IR::Opcode::SetZFlag:
        case IR::Opcode::SetCFlag:
        case IR::Opcode::SetVFlag:
        case IR::Opcode::OrQFlag:
        case IR::Opcode::BXWritePC:
        case IR::Opcode::CallSupervisor:
        case IR::Opcode::PushRSB:
        case IR::Opcode::FPAbs32:
        case IR::Opcode::FPAbs64:
        case IR::Opcode::FPAdd32:
        case IR::Opcode::FPAdd64:
        case IR::Opcode::FPDiv32:
        case IR::Opcode::FPDiv64:
        case IR::Opcode::FPMul32:
        case IR::Opcode::FPMul64:
        case IR::Opcode::FPNeg32:
        case IR::Opcode::FPNeg64:
        case IR::Opcode::FPSqrt32:
        case IR::Opcode::FPSqrt64:
        case IR::Opcode::FPSub32:
        case IR::Opcode::FPSub64:
        case IR::Opcode::ClearExclusive:
        case IR::Opcode::SetExclusive:
        case IR::Opcode::WriteMemory8:
        case IR::Opcode::WriteMemory16:
        case IR::Opcode::WriteMemory32:
        case IR::Opcode::WriteMemory64:
        case IR::Opcode::ExclusiveWriteMemory8:
        case IR::Opcode::ExclusiveWriteMemory16:
        case IR::Opcode::ExclusiveWriteMemory32:
        case IR::Opcode::ExclusiveWriteMemory64:
            return false;
        default:
            ASSERT(IR::GetTypeOf(op) != IR::Type::Void);
            return true;
        }
    };

    // We iterate over the instructions in reverse order.
    // This is because removing an instruction reduces the number of uses for earlier instructions.

    if (block.empty()) {
        return;
    }

    auto iter = block.end();
    do {
        --iter;
        if (!iter->HasUses() && is_side_effect_free(iter->GetOpcode())) {
            iter->Invalidate();
            iter = block.instructions.erase(iter);
        }
    } while (iter != block.begin());
}

} // namespace Optimization
} // namespace Dynarmic
