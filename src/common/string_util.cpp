/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <cstdio>
#include <cstdarg>
#include <string>

#include "common/string_util.h"

namespace Dynarmic {
namespace Common {

std::string StringFromFormat(const char* format, ...) {
    va_list args;

    va_start(args, format);
    int size = std::vsnprintf(nullptr, 0, format, args);
    va_end(args);

    std::string str;
    str.resize(size+1);

    va_start(args, format);
    std::vsnprintf(&str[0], str.size(), format, args);
    va_end(args);

    str.resize(size);

    return str;
}

} // namespace Common
} // namespace Dynarmic
