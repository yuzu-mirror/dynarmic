/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "backend/x64/exception_handler.h"

namespace Dynarmic::Backend::X64 {

struct ExceptionHandler::Impl final {
};

ExceptionHandler::ExceptionHandler() = default;
ExceptionHandler::~ExceptionHandler() = default;

void ExceptionHandler::Register(BlockOfCode&) {
    // Do nothing
}

bool ExceptionHandler::SupportsFastmem() const noexcept {
    return false;
}

void ExceptionHandler::SetFastmemCallback(std::function<FakeCall(u64)>) {
    // Do nothing
}

} // namespace Dynarmic::Backend::X64
