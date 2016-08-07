/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <set>
#include <unordered_map>

#include "backend_x64/block_of_code.h"
#include "backend_x64/reg_alloc.h"
#include "common/x64/emitter.h"
#include "frontend/ir/ir.h"
#include "interface/interface.h"

namespace Dynarmic {
namespace BackendX64 {

class EmitX64 final {
public:
    EmitX64(BlockOfCode* code, UserCallbacks cb, Jit* jit_interface)
            : reg_alloc(code), code(code), cb(cb), jit_interface(jit_interface) {}

    struct BlockDescriptor {
        CodePtr code_ptr;
        size_t size;
    };
    BlockDescriptor* Emit(const Arm::LocationDescriptor descriptor, IR::Block& ir);

    BlockDescriptor* GetBasicBlock(Arm::LocationDescriptor descriptor) {
        auto iter = basic_blocks.find(descriptor);
        return iter != basic_blocks.end() ? &iter->second : nullptr;
    }

    void ClearCache();

private:
    // Microinstruction emitters
#define OPCODE(name, type, ...) void Emit##name(IR::Block& block, IR::Inst* inst);
#include "frontend/ir/opcodes.inc"
#undef OPCODE

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
    BlockOfCode* code;
    UserCallbacks cb;
    Jit* jit_interface;
    std::unordered_map<Arm::LocationDescriptor, BlockDescriptor, Arm::LocationDescriptorHash> basic_blocks;
};

} // namespace BackendX64
} // namespace Dynarmic
