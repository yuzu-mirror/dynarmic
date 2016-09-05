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

IR::Block TranslateArm(IR::LocationDescriptor descriptor, MemoryRead32FuncType memory_read_32);
IR::Block TranslateThumb(IR::LocationDescriptor descriptor, MemoryRead32FuncType memory_read_32);

IR::Block Translate(IR::LocationDescriptor descriptor, MemoryRead32FuncType memory_read_32) {
    return (descriptor.TFlag() ? TranslateThumb : TranslateArm)(descriptor, memory_read_32);
}

} // namespace Arm
} // namespace Dynarmic
