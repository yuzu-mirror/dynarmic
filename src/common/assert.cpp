/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <cstdio>
#include <exception>

#include <fmt/format.h>

namespace Dynarmic::Common {

[[noreturn]] void Terminate(fmt::string_view msg, fmt::format_args args) {
    fmt::print(stderr, "dynarmic assertion failed: ");
    fmt::vprint(stderr, msg, args);
    std::terminate();
}

} // namespace Dynarmic::Common
