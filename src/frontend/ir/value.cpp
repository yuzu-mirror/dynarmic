/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/ir/microinstruction.h"
#include "frontend/ir/value.h"

namespace Dynarmic {
namespace IR {

Value::Value(Inst* value) : type(Type::Opaque) {
    inner.inst = value;
}

Value::Value(Arm::Reg value) : type(Type::RegRef) {
    inner.imm_regref = value;
}

Value::Value(Arm::ExtReg value) : type(Type::ExtRegRef) {
    inner.imm_extregref = value;
}

Value::Value(bool value) : type(Type::U1) {
    inner.imm_u1 = value;
}

Value::Value(u8 value) : type(Type::U8) {
    inner.imm_u8 = value;
}

Value::Value(u32 value) : type(Type::U32) {
    inner.imm_u32 = value;
}

Value::Value(u64 value) : type(Type::U64) {
    inner.imm_u64 = value;
}

bool Value::IsImmediate() const {
    if (type == Type::Opaque)
        return inner.inst->GetOpcode() == Opcode::Identity ? inner.inst->GetArg(0).IsImmediate() : false;
    return true;
}

bool Value::IsEmpty() const {
    return type == Type::Void;
}

Type Value::GetType() const {
    if (type == Type::Opaque) {
        if (inner.inst->GetOpcode() == Opcode::Identity) {
            return inner.inst->GetArg(0).GetType();
        } else {
            return inner.inst->GetType();
        }
    }
    return type;
}

Arm::Reg Value::GetRegRef() const {
    DEBUG_ASSERT(type == Type::RegRef);
    return inner.imm_regref;
}

Arm::ExtReg Value::GetExtRegRef() const {
    DEBUG_ASSERT(type == Type::ExtRegRef);
    return inner.imm_extregref;
}

Inst* Value::GetInst() const {
    DEBUG_ASSERT(type == Type::Opaque);
    return inner.inst;
}

bool Value::GetU1() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU1();
    DEBUG_ASSERT(type == Type::U1);
    return inner.imm_u1;
}

u8 Value::GetU8() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU8();
    DEBUG_ASSERT(type == Type::U8);
    return inner.imm_u8;
}

u32 Value::GetU32() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU32();
    DEBUG_ASSERT(type == Type::U32);
    return inner.imm_u32;
}

u64 Value::GetU64() const {
    if (type == Type::Opaque && inner.inst->GetOpcode() == Opcode::Identity)
        return inner.inst->GetArg(0).GetU64();
    DEBUG_ASSERT(type == Type::U64);
    return inner.imm_u64;
}

} // namespace IR
} // namespace Dynarmic
