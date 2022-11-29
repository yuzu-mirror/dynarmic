/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "dynarmic/backend/arm64/address_space.h"
#include "dynarmic/interface/A32/config.h"

namespace Dynarmic::Backend::Arm64 {

class A32AddressSpace final : public AddressSpace {
public:
    explicit A32AddressSpace(const A32::UserConfig& conf);

    IR::Block GenerateIR(IR::LocationDescriptor) const override;

protected:
    friend class A32Core;

    void EmitPrelude();
    EmitConfig GetEmitConfig() override;

    const A32::UserConfig conf;
};

}  // namespace Dynarmic::Backend::Arm64
