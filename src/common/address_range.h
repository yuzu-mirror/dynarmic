/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <cstddef>

#include <boost/variant.hpp>

#include "common/common_types.h"

namespace Dynarmic {
namespace Common {

struct FullAddressRange {};

struct AddressInterval {
    u32 start_address;
    std::size_t length;

    // Does this interval overlap with [from, to)?
    bool Overlaps(u32 from, u32 to) const {
        return start_address <= to && from <= start_address + length;
    }
};

using AddressRange = boost::variant<FullAddressRange, AddressInterval>;

} // namespace Common
} // namespace Dynarmic
