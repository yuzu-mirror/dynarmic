/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <functional>
#include <memory>

#include "common/common_types.h"

namespace Dynarmic::Backend::X64 {

class BlockOfCode;

struct FakeCall {
    u64 call_rip;
    u64 ret_rip;
};

class ExceptionHandler final {
public:
    ExceptionHandler();
    ~ExceptionHandler();

    void Register(BlockOfCode& code);

    bool SupportsFastmem() const noexcept;
    void SetFastmemCallback(std::function<FakeCall(u64)> cb);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Dynarmic::Backend::X64
