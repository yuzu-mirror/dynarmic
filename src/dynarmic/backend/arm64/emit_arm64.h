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
}  // namespace oaknut

namespace Dynarmic::IR {
class Block;
}  // namespace Dynarmic::IR

namespace Dynarmic::Backend::Arm64 {

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

EmittedBlockInfo EmitArm64(oaknut::CodeGenerator& code, IR::Block block);

}  // namespace Dynarmic::Backend::Arm64
