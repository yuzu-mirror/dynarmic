/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <string>

namespace Dynarmic {
namespace Common {

std::string StringFromFormat(
#ifdef _MSC_VER
    _Printf_format_string_
#endif
    const char* format, ...)
#if defined(__GNUC__) && !defined(__clang__)
    __attribute__((format(gnu_printf, 1, 2)))
#endif
    ;

} // namespace Common
} // namespace Dynarmic
