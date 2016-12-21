/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "frontend/ir/basic_block.h"
#include "frontend/ir/location_descriptor.h"
#include "frontend/translate/translate.h"

namespace Dynarmic {
namespace Arm {

IR::Block TranslateArm(IR::LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code);
IR::Block TranslateThumb(IR::LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code);

IR::Block Translate(IR::LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code) {
    return (descriptor.TFlag() ? TranslateThumb : TranslateArm)(descriptor, memory_read_code);
}

} // namespace Arm
} // namespace Dynarmic
