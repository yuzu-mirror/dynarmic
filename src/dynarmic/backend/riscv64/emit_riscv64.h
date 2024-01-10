/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <vector>

#include <mcl/stdint.hpp>

namespace biscuit {
class Assembler;
}  // namespace biscuit

namespace Dynarmic::IR {
class Block;
enum class Opcode;
class Inst;
}  // namespace Dynarmic::IR

namespace Dynarmic::Backend::RV64 {

using CodePtr = std::byte*;

enum class LinkTarget {
    ReturnFromRunCode,
};

struct Relocation {
    std::ptrdiff_t code_offset;
    LinkTarget target;
};

struct EmittedBlockInfo {
    CodePtr entry_point;
    size_t size;
    std::vector<Relocation> relocations;
};

struct EmitConfig {};

struct EmitContext;

template<IR::Opcode op>
void EmitIR(biscuit::Assembler& as, EmitContext& ctx, IR::Inst* inst);

EmittedBlockInfo EmitRV64(biscuit::Assembler& as, IR::Block block, const EmitConfig& emit_conf);

}  // namespace Dynarmic::Backend::RV64
