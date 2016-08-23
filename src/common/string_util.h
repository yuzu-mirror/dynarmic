/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <string>

namespace Dynarmic {
namespace Common {

#ifdef __MINGW32__
[[gnu::format(gnu_printf, 1, 2)]]
#elif !defined(_MSC_VER)
[[gnu::format(printf, 1, 2)]]
#endif
std::string StringFromFormat(
#ifdef _MSC_VER
    _Printf_format_string_
#endif
    const char* format, ...);

template <typename T>
constexpr char SignToChar(T value) {
    return value >= 0 ? '+' : '-';
}

} // namespace Common
} // namespace Dynarmic
