/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/assert.h"
#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::asimd_VEXT(bool D, size_t Vn, size_t Vd, Imm<4> imm4, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (!Q && imm4.Bit<3>()) {
        return UndefinedInstruction();
    }

    const u8 position = 8 * imm4.ZeroExtend<u8>();
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto result = Q ? ir.VectorExtract(reg_n, reg_m, position) : ir.VectorExtractLower(reg_n, reg_m, position);

    ir.SetVector(d, result);
    return true;
}

} // namespace Dynarmic::A32
