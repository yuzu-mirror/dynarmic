/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

static bool InstructionCacheInstruction(TranslatorVisitor& v, InstructionCacheOperation op, const Reg Rt) {
    v.ir.InstructionCacheOperationRaised(op, v.X(64, Rt));
    return true;
}

bool TranslatorVisitor::IC_IALLU() {
    return false;
}

bool TranslatorVisitor::IC_IALLUIS() {
    return false;
}

bool TranslatorVisitor::IC_IVAU(Reg Rt) {
    return InstructionCacheInstruction(*this, InstructionCacheOperation::InvalidateByVAToPoU, Rt);
}

} // namespace Dynarmic::A64
