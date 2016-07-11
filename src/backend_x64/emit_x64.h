/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <set>
#include <unordered_map>

#include "backend_x64/reg_alloc.h"
#include "backend_x64/routines.h"
#include "common/x64/emitter.h"
#include "frontend/ir/ir.h"
#include "interface/interface.h"

namespace Dynarmic {
namespace BackendX64 {

class EmitX64 final {
public:
    EmitX64(Gen::XEmitter* code, Routines* routines, UserCallbacks cb, Jit* jit_interface)
            : reg_alloc(code), code(code), routines(routines), cb(cb), jit_interface(jit_interface) {}

    CodePtr Emit(Arm::LocationDescriptor descriptor, IR::Block ir);

    CodePtr GetBasicBlock(Arm::LocationDescriptor descriptor) {
        auto iter = basic_blocks.find(descriptor);
        return iter != basic_blocks.end() ? iter->second : nullptr;
    }

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
    void EmitGetOverflowFromOp(IR::Value* value);
    void EmitLeastSignificantHalf(IR::Value* value);
    void EmitLeastSignificantByte(IR::Value* value);
    void EmitMostSignificantBit(IR::Value* value);
    void EmitIsZero(IR::Value* value);
    void EmitLogicalShiftLeft(IR::Value* value);
    void EmitLogicalShiftRight(IR::Value* value);
    void EmitArithmeticShiftRight(IR::Value* value);
    void EmitRotateRight(IR::Value* value);
    void EmitAddWithCarry(IR::Value* value);
    void EmitSubWithCarry(IR::Value* value);
    void EmitAnd(IR::Value* value);
    void EmitEor(IR::Value* value);
    void EmitOr(IR::Value* value);
    void EmitNot(IR::Value* value);
    void EmitReadMemory8(IR::Value* value);
    void EmitReadMemory16(IR::Value* value);
    void EmitReadMemory32(IR::Value* value);
    void EmitReadMemory64(IR::Value* value);
    void EmitWriteMemory8(IR::Value* value);
    void EmitWriteMemory16(IR::Value* value);
    void EmitWriteMemory32(IR::Value* value);
    void EmitWriteMemory64(IR::Value* value);

    void EmitAddCycles(size_t cycles);

    void EmitTerminal(IR::Terminal terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalInterpret(IR::Term::Interpret terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalReturnToDispatch(IR::Term::ReturnToDispatch terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalLinkBlock(IR::Term::LinkBlock terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalLinkBlockFast(IR::Term::LinkBlockFast terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalPopRSBHint(IR::Term::PopRSBHint terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalIf(IR::Term::If terminal, Arm::LocationDescriptor initial_location);

    void ClearCache();

private:
    std::set<IR::Value*> inhibit_emission;
    RegAlloc reg_alloc;

    Gen::XEmitter* code;
    Routines* routines;
    UserCallbacks cb;
    Jit* jit_interface;
    std::unordered_map<Arm::LocationDescriptor, CodePtr, Arm::LocationDescriptorHash> basic_blocks;
};

} // namespace BackendX64
} // namespace Dynarmic
