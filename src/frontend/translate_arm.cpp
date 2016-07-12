/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/arm_types.h"
#include "frontend/ir/ir.h"
#include "frontend/ir_emitter.h"
#include "frontend/translate.h"

namespace Dynarmic {
namespace Arm {

IR::Block TranslateArm(LocationDescriptor descriptor, MemoryRead32FuncType memory_read_32) {
    // Just interpret everything.
    IREmitter ir{descriptor};
    ir.SetTerm(IR::Term::Interpret{ir.current_location});
    ir.block.cycle_count++;
    return ir.block;
}

} // namespace Arm
} // namespace Dynarmic
