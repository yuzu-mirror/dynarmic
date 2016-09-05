/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "common/common_types.h"
#include "frontend/arm/types.h"

namespace Dynarmic {
namespace IR {

class Inst;

/**
 * A representation of a value in the IR.
 * A value may either be an immediate or the result of a microinstruction.
 */
class Value final {
public:
    Value() : type(Type::Void) {}
    explicit Value(Inst* value);
    explicit Value(Arm::Reg value);
    explicit Value(Arm::ExtReg value);
    explicit Value(bool value);
    explicit Value(u8 value);
    explicit Value(u32 value);
    explicit Value(u64 value);

    bool IsEmpty() const;
    bool IsImmediate() const;
    Type GetType() const;

    Inst* GetInst() const;
    Arm::Reg GetRegRef() const;
    Arm::ExtReg GetExtRegRef() const;
    bool GetU1() const;
    u8 GetU8() const;
    u32 GetU32() const;
    u64 GetU64() const;

private:
    Type type;

    union {
        Inst* inst; // type == Type::Opaque
        Arm::Reg imm_regref;
        Arm::ExtReg imm_extregref;
        bool imm_u1;
        u8 imm_u8;
        u32 imm_u32;
        u64 imm_u64;
    } inner;
};

} // namespace IR
} // namespace Dynarmic
