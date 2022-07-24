/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstddef>
#include <vector>

#include <mcl/stdint.hpp>

namespace oaknut {
struct PointerCodeGeneratorPolicy;
template<typename>
class BasicCodeGenerator;
using CodeGenerator = BasicCodeGenerator<PointerCodeGeneratorPolicy>;
struct Label;
}  // namespace oaknut

namespace Dynarmic::IR {
class Block;
class Inst;
enum class Cond;
enum class Opcode;
}  // namespace Dynarmic::IR

namespace Dynarmic::Backend::Arm64 {

using CodePtr = std::byte*;

enum class LinkTarget {
    ReturnFromRunCode,
    ReadMemory8,
    ReadMemory16,
    ReadMemory32,
    ReadMemory64,
    WriteMemory8,
    WriteMemory16,
    WriteMemory32,
    WriteMemory64,
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

struct EmitConfig {
    bool enable_cycle_counting;
    bool always_little_endian;
};

struct EmitContext;

EmittedBlockInfo EmitArm64(oaknut::CodeGenerator& code, IR::Block block, const EmitConfig& emit_conf);

template<IR::Opcode op>
void EmitIR(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst);
void EmitRelocation(oaknut::CodeGenerator& code, EmitContext& ctx, LinkTarget link_target);
oaknut::Label EmitA32Cond(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Cond cond);
void EmitA32Terminal(oaknut::CodeGenerator& code, EmitContext& ctx);

}  // namespace Dynarmic::Backend::Arm64
