/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <ostream>
#include <fmt/format.h>
#include "frontend/A32/location_descriptor.h"

namespace Dynarmic::A32 {

std::ostream& operator<<(std::ostream& o, const LocationDescriptor& descriptor) {
    o << fmt::format("{{{:08x},{},{},{:08x}{}}}",
                     descriptor.PC(),
                     descriptor.TFlag() ? "T" : "!T",
                     descriptor.EFlag() ? "E" : "!E",
                     descriptor.FPSCR().Value(),
                     descriptor.SingleStepping() ? ",step" : "");
    return o;
}

} // namespace Dynarmic::A32
