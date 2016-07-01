/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <cstdarg>
#include <cstdio>

#include "common/logging/log.h"

namespace Log {

void LogMessage(Class log_class, Level log_level, const char* filename, unsigned int line_nr, const char* function, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

} // namespace Log
