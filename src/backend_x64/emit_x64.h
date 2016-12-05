/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

#include <xbyak_util.h>

#include "backend_x64/reg_alloc.h"
#include "dynarmic/callbacks.h"
#include "frontend/ir/location_descriptor.h"
#include "frontend/ir/terminal.h"

namespace Dynarmic {

class Jit;

namespace IR {
class Block;
class Inst;
} // namespace IR

namespace BackendX64 {

class BlockOfCode;

class EmitX64 final {
public:
    struct BlockDescriptor {
        CodePtr code_ptr; ///< Entrypoint of emitted code
        size_t size;      ///< Length in bytes of emitted code
    };

    EmitX64(BlockOfCode* code, UserCallbacks cb, Jit* jit_interface);

    /**
     * Emit host machine code for a basic block with intermediate representation `ir`.
     * @note ir is modified.
     */
    BlockDescriptor Emit(IR::Block& ir);

    /// Looks up an emitted host block in the cache.
    boost::optional<BlockDescriptor> GetBasicBlock(IR::LocationDescriptor descriptor) const;

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
    void EmitTerminal(IR::Terminal terminal, IR::LocationDescriptor initial_location);
    void EmitTerminalInterpret(IR::Term::Interpret terminal, IR::LocationDescriptor initial_location);
    void EmitTerminalReturnToDispatch(IR::Term::ReturnToDispatch terminal, IR::LocationDescriptor initial_location);
    void EmitTerminalLinkBlock(IR::Term::LinkBlock terminal, IR::LocationDescriptor initial_location);
    void EmitTerminalLinkBlockFast(IR::Term::LinkBlockFast terminal, IR::LocationDescriptor initial_location);
    void EmitTerminalPopRSBHint(IR::Term::PopRSBHint terminal, IR::LocationDescriptor initial_location);
    void EmitTerminalIf(IR::Term::If terminal, IR::LocationDescriptor initial_location);
    void EmitTerminalCheckHalt(IR::Term::CheckHalt terminal, IR::LocationDescriptor initial_location);
    void Patch(IR::LocationDescriptor desc, CodePtr bb);

    // Global CPU information
    Xbyak::util::Cpu cpu_info;

    // Per-block state
    RegAlloc reg_alloc;

    // State
    BlockOfCode* code;
    UserCallbacks cb;
    Jit* jit_interface;
    std::unordered_map<u64, CodePtr> unique_hash_to_code_ptr;
    std::unordered_map<u64, std::vector<CodePtr>> patch_unique_hash_locations;
    std::unordered_map<IR::LocationDescriptor, BlockDescriptor> basic_blocks;
    std::unordered_map<IR::LocationDescriptor, std::vector<CodePtr>> patch_jg_locations;
    std::unordered_map<IR::LocationDescriptor, std::vector<CodePtr>> patch_jmp_locations;
};

} // namespace BackendX64
} // namespace Dynarmic
