/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/arm_types.h"
#include "frontend/ir/ir.h"
#include "frontend/translate.h"

namespace Dynarmic {
namespace Arm {

IR::Block TranslateArm(LocationDescriptor descriptor, MemoryRead32FuncType memory_read_32) {
    ASSERT_MSG(0, "Unimplemented");
    return IR::Block(descriptor);
}

} // namespace Arm
} // namespace Dynarmic
