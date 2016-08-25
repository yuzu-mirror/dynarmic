/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <set>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>
#include <xbyak.h>

#include "backend_x64/block_of_code.h"
#include "backend_x64/reg_alloc.h"
#include "frontend/arm_types.h"
#include "frontend/ir/basic_block.h"
#include "frontend/ir/microinstruction.h"
#include "frontend/ir/terminal.h"
#include "interface/interface.h"

namespace Dynarmic {
namespace BackendX64 {

class EmitX64 final {
public:
    EmitX64(BlockOfCode* code, UserCallbacks cb, Jit* jit_interface)
            : reg_alloc(code), code(code), cb(cb), jit_interface(jit_interface) {}

    struct BlockDescriptor {
        CodePtr code_ptr; ///< Entrypoint of emitted code
        size_t size;      ///< Length in bytes of emitted code
    };

    /// Emit host machine code for a basic block starting at `descriptor` with intermediate representation `ir`.
    BlockDescriptor Emit(const Arm::LocationDescriptor descriptor, IR::Block& ir);

    /// Looks up an emitted host block in the cache.
    boost::optional<BlockDescriptor> GetBasicBlock(Arm::LocationDescriptor descriptor) {
        auto iter = basic_blocks.find(descriptor);
        if (iter == basic_blocks.end())
            return boost::none;
        return boost::make_optional<BlockDescriptor>(iter->second);
    }

    /// Empties the cache.
    void ClearCache();

private:
    // Microinstruction emitters
#define OPCODE(name, type, ...) void Emit##name(IR::Block& block, IR::Inst* inst);
#include "frontend/ir/opcodes.inc"
#undef OPCODE

    // Helpers
    void EmitAddCycles(size_t cycles);
    void EmitCondPrelude(const IR::Block& block);

    // Terminal instruction emitters
    void EmitTerminal(IR::Terminal terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalInterpret(IR::Term::Interpret terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalReturnToDispatch(IR::Term::ReturnToDispatch terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalLinkBlock(IR::Term::LinkBlock terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalLinkBlockFast(IR::Term::LinkBlockFast terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalPopRSBHint(IR::Term::PopRSBHint terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalIf(IR::Term::If terminal, Arm::LocationDescriptor initial_location);
    void EmitTerminalCheckHalt(IR::Term::CheckHalt terminal, Arm::LocationDescriptor initial_location);
    void Patch(Arm::LocationDescriptor desc, CodePtr bb);

    // Per-block state
    RegAlloc reg_alloc;

    // State
    BlockOfCode* code;
    UserCallbacks cb;
    Jit* jit_interface;
    std::unordered_map<u64, CodePtr> unique_hash_to_code_ptr;
    std::unordered_map<u64, std::vector<CodePtr>> patch_unique_hash_locations;
    std::unordered_map<Arm::LocationDescriptor, BlockDescriptor, Arm::LocationDescriptorHash> basic_blocks;
    std::unordered_map<Arm::LocationDescriptor, std::vector<CodePtr>, Arm::LocationDescriptorHash> patch_jg_locations;
    std::unordered_map<Arm::LocationDescriptor, std::vector<CodePtr>, Arm::LocationDescriptorHash> patch_jmp_locations;
};

} // namespace BackendX64
} // namespace Dynarmic
