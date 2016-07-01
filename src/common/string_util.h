/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <string>

namespace Dynarmic {
namespace Common {

std::string StringFromFormat(const char* format, ...) __attribute__((format(gnu_printf, 1, 2)));

} // namespace Common
} // namespace Dynarmic
