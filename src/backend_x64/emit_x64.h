/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <set>

#include "backend_x64/reg_alloc.h"
#include "backend_x64/routines.h"
#include "common/x64/emitter.h"
#include "frontend/ir/ir.h"
#include "interface/interface.h"

namespace Dynarmic {
namespace BackendX64 {

class EmitX64 final {
public:
    EmitX64(Gen::XEmitter* code, Routines* routines, UserCallbacks cb) : code(code), reg_alloc(code), routines(routines), cb(cb) {}

    CodePtr Emit(Arm::LocationDescriptor descriptor, IR::Block ir);
    CodePtr GetBasicBlock(Arm::LocationDescriptor descriptor);

    void EmitImmU1(IR::Value* value);
    void EmitImmU8(IR::Value* value);
    void EmitImmU32(IR::Value* value);
    void EmitImmRegRef(IR::Value* value);
    void EmitGetRegister(IR::Value* value);
    void EmitSetRegister(IR::Value* value);
    void EmitGetNFlag(IR::Value* value);
    void EmitSetNFlag(IR::Value* value);
    void EmitGetZFlag(IR::Value* value);
    void EmitSetZFlag(IR::Value* value);
    void EmitGetCFlag(IR::Value* value);
    void EmitSetCFlag(IR::Value* value);
    void EmitGetVFlag(IR::Value* value);
    void EmitSetVFlag(IR::Value* value);
    void EmitGetCarryFromOp(IR::Value* value);
    void EmitLeastSignificantByte(IR::Value* value);
    void EmitMostSignificantBit(IR::Value* value);
    void EmitIsZero(IR::Value* value);
    void EmitLogicalShiftLeft(IR::Value* value);
    void EmitLogicalShiftRight(IR::Value* value);
    void EmitArithmeticShiftRight(IR::Value* value);

    void EmitReturnToDispatch();

private:
    std::set<IR::Value*> inhibit_emission;
    Gen::XEmitter* code;
    RegAlloc reg_alloc;
    Routines* routines;
    UserCallbacks cb;
};

} // namespace BackendX64
} // namespace Dynarmic
