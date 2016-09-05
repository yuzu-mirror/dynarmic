/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <ostream>
#include <fmt/format.h>
#include "frontend/ir/location_descriptor.h"

namespace Dynarmic {
namespace IR {

std::ostream& operator<<(std::ostream& o, const LocationDescriptor& loc) {
    o << fmt::format("{{{},{},{},{}}}",
                     loc.PC(),
                     loc.TFlag() ? "T" : "!T",
                     loc.EFlag() ? "E" : "!E",
                     loc.FPSCR().Value());
    return o;
}

} // namespace IR
} // namespace Dynarmic
