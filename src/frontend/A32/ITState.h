/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "common/common_types.h"
#include "common/bit_util.h"
#include "frontend/ir/cond.h"

namespace Dynarmic::A32 {

class ITState final {
public:
    ITState() = default;
    explicit ITState(u8 data) : value(data) {}

    ITState& operator=(u8 data) {
        value = data;
        return *this;
    }

    IR::Cond Cond() const {
        return static_cast<IR::Cond>(Common::Bits<4, 7>(value));
    }
    void Cond(IR::Cond cond) {
        value = Common::ModifyBits<4, 7>(value, static_cast<u8>(cond));
    }

    u8 Mask() const {
        return Common::Bits<0, 3>(value);
    }
    void Mask(u8 mask) {
        value = Common::ModifyBits<0, 3>(value, mask);
    }

    bool IsInITBlock() const {
        return Mask() != 0b0000;
    }

    bool IsLastInITBlock() const {
        return Mask() == 0b1000;
    }

    ITState Advance() const {
        ITState result{*this};
        result.Mask(result.Mask() << 1);
        if (result.Mask() == 0) {
            return ITState{0};
        }
        return result;
    }

    u8 Value() const {
        return value;
    }

private:
    u8 value;
};

inline bool operator==(ITState lhs, ITState rhs) {
    return lhs.Value() == rhs.Value();
}

inline bool operator!=(ITState lhs, ITState rhs) {
    return !operator==(lhs, rhs);
}

} // namespace Dynarmic::A32
