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

    CodePtr Emit(const Arm::LocationDescriptor descriptor, IR::Block& ir);

    CodePtr GetBasicBlock(Arm::LocationDescriptor descriptor) {
        auto iter = basic_blocks.find(descriptor);
        return iter != basic_blocks.end() ? iter->second : nullptr;
    }

    void ClearCache();

private:
    // Microinstruction emitters
    void EmitIdentity(IR::Block& block, IR::Inst* inst);
    void EmitGetRegister(IR::Block& block, IR::Inst* inst);
    void EmitSetRegister(IR::Block& block, IR::Inst* inst);
    void EmitGetNFlag(IR::Block& block, IR::Inst* inst);
    void EmitSetNFlag(IR::Block& block, IR::Inst* inst);
    void EmitGetZFlag(IR::Block& block, IR::Inst* inst);
    void EmitSetZFlag(IR::Block& block, IR::Inst* inst);
    void EmitGetCFlag(IR::Block& block, IR::Inst* inst);
    void EmitSetCFlag(IR::Block& block, IR::Inst* inst);
    void EmitGetVFlag(IR::Block& block, IR::Inst* inst);
    void EmitSetVFlag(IR::Block& block, IR::Inst* inst);
    void EmitBXWritePC(IR::Block& block, IR::Inst* inst);
    void EmitCallSupervisor(IR::Block& block, IR::Inst* inst);
    void EmitGetCarryFromOp(IR::Block& block, IR::Inst* inst);
    void EmitGetOverflowFromOp(IR::Block& block, IR::Inst* inst);
    void EmitLeastSignificantHalf(IR::Block& block, IR::Inst* inst);
    void EmitLeastSignificantByte(IR::Block& block, IR::Inst* inst);
    void EmitMostSignificantBit(IR::Block& block, IR::Inst* inst);
    void EmitIsZero(IR::Block& block, IR::Inst* inst);
    void EmitLogicalShiftLeft(IR::Block& block, IR::Inst* inst);
    void EmitLogicalShiftRight(IR::Block& block, IR::Inst* inst);
    void EmitArithmeticShiftRight(IR::Block& block, IR::Inst* inst);
    void EmitRotateRight(IR::Block& block, IR::Inst* inst);
    void EmitAddWithCarry(IR::Block& block, IR::Inst* inst);
    void EmitSubWithCarry(IR::Block& block, IR::Inst* inst);
    void EmitAnd(IR::Block& block, IR::Inst* inst);
    void EmitEor(IR::Block& block, IR::Inst* inst);
    void EmitOr(IR::Block& block, IR::Inst* inst);
    void EmitNot(IR::Block& block, IR::Inst* inst);
    void EmitSignExtendHalfToWord(IR::Block& block, IR::Inst* inst);
    void EmitSignExtendByteToWord(IR::Block& block, IR::Inst* inst);
    void EmitZeroExtendHalfToWord(IR::Block& block, IR::Inst* inst);
    void EmitZeroExtendByteToWord(IR::Block& block, IR::Inst* inst);
    void EmitByteReverseWord(IR::Block& block, IR::Inst* inst);
    void EmitByteReverseHalf(IR::Block& block, IR::Inst* inst);
    void EmitByteReverseDual(IR::Block& block, IR::Inst* inst);
    void EmitReadMemory8(IR::Block& block, IR::Inst* inst);
    void EmitReadMemory16(IR::Block& block, IR::Inst* inst);
    void EmitReadMemory32(IR::Block& block, IR::Inst* inst);
    void EmitReadMemory64(IR::Block& block, IR::Inst* inst);
    void EmitWriteMemory8(IR::Block& block, IR::Inst* inst);
    void EmitWriteMemory16(IR::Block& block, IR::Inst* inst);
    void EmitWriteMemory32(IR::Block& block, IR::Inst* inst);
    void EmitWriteMemory64(IR::Block& block, IR::Inst* inst);

    // Helpers
    void EmitAddCycles(size_t cycles);
    void EmitCondPrelude(Arm::Cond cond,
                         boost::optional<Arm::LocationDescriptor> cond_failed,
                         Arm::LocationDescriptor current_location);

    // Terminal instruction emitters
    void EmitTerminal(IR::Terminal terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalInterpret(IR::Term::Interpret terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalReturnToDispatch(IR::Term::ReturnToDispatch terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalLinkBlock(IR::Term::LinkBlock terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalLinkBlockFast(IR::Term::LinkBlockFast terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalPopRSBHint(IR::Term::PopRSBHint terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalIf(IR::Term::If terminal, Arm::LocationDescriptor initial_location);

    // Per-block state
    std::set<IR::Value*> inhibit_emission;
    RegAlloc reg_alloc;

    // State
    Gen::XEmitter* code;
    Routines* routines;
    UserCallbacks cb;
    Jit* jit_interface;
    std::unordered_map<Arm::LocationDescriptor, CodePtr, Arm::LocationDescriptorHash> basic_blocks;
};

} // namespace BackendX64
} // namespace Dynarmic
