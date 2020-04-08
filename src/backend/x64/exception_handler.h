/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <memory>

namespace Dynarmic::Backend::X64 {

class BlockOfCode;

class ExceptionHandler final {
public:
    ExceptionHandler();
    ~ExceptionHandler();

    void Register(BlockOfCode& code);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Dynarmic::Backend::X64
