/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <functional>
#include <vector>

#include <xbyak.h>

#include "common/common_types.h"

namespace Dynarmic::Backend::X64 {

using RegList = std::vector<Xbyak::Reg64>;

class BlockOfCode;

class Callback {
public:
    virtual ~Callback();

    virtual void EmitCall(BlockOfCode& code, std::function<void(RegList)> fn = [](RegList){}) const = 0;
    virtual void EmitCallWithReturnPointer(BlockOfCode& code, std::function<void(Xbyak::Reg64, RegList)> fn) const = 0;
};

class SimpleCallback final : public Callback {
public:
    template <typename Function>
    SimpleCallback(Function fn) : fn(reinterpret_cast<void(*)()>(fn)) {}

    void EmitCall(BlockOfCode& code, std::function<void(RegList)> fn = [](RegList){}) const override;
    void EmitCallWithReturnPointer(BlockOfCode& code, std::function<void(Xbyak::Reg64, RegList)> fn) const override;

private:
    void (*fn)();
};

class ArgCallback final : public Callback {
public:
    template <typename Function>
    ArgCallback(Function fn, u64 arg) : fn(reinterpret_cast<void(*)()>(fn)), arg(arg) {}

    void EmitCall(BlockOfCode& code, std::function<void(RegList)> fn = [](RegList){}) const override;
    void EmitCallWithReturnPointer(BlockOfCode& code, std::function<void(Xbyak::Reg64, RegList)> fn) const override;

private:
    void (*fn)();
    u64 arg;
};

} // namespace Dynarmic::Backend::X64
