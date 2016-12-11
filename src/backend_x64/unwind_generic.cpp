/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "backend_x64/block_of_code.h"

namespace Dynarmic {
namespace BackendX64 {

struct BlockOfCode::UnwindHandler::Impl final {
};

BlockOfCode::UnwindHandler::UnwindHandler() = default;
BlockOfCode::UnwindHandler::~UnwindHandler() = default;

void BlockOfCode::UnwindHandler::Register(BlockOfCode*) {
    // Do nothing
}

} // namespace BackendX64
} // namespace Dynarmic
