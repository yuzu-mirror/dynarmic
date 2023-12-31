/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/stdint.hpp>
#include <tsl/robin_map.h>

namespace biscuit {
class Assembler;
}  // namespace biscuit

namespace Dynarmic::IR {
class Block;
}  // namespace Dynarmic::IR

namespace Dynarmic::Backend::RV64 {

enum class LinkTarget {
    ReturnFromRunCode,
};

struct EmittedBlockInfo {
    void* entry_point;
    size_t size;
    tsl::robin_map<std::ptrdiff_t, LinkTarget> relocations;
};

EmittedBlockInfo EmitRV64(biscuit::Assembler& as, IR::Block block);

}  // namespace Dynarmic::Backend::RV64
