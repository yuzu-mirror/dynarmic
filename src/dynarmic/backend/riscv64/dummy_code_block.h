/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

namespace Dynarmic::Backend::RV64 {

class DummyCodeBlock {
public:
    DummyCodeBlock() {}

    void* ptr() { return nullptr; }
};
}  // namespace Dynarmic::Backend::RV64
