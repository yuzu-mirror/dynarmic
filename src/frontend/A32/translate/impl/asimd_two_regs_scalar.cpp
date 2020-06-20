/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/assert.h"
#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::asimd_VMUL_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool F, bool N, bool M, size_t Vm) {
    ASSERT_MSG(sz != 0b11, "Decode error");
    if (sz == 0b00 || (F && sz == 0b01)) {
        return UndefinedInstruction();
    }
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn))) {
        return UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto n = ToVector(Q, Vn, N);
    const size_t esize = 8u << sz;

    const auto m = ExtReg::Q0 + ((Vm >> 1) & (esize == 16 ? 0b11 : 0b111));
    const auto index = concatenate(Imm<1>{Common::Bit<0>(Vm)}, Imm<1>{M}, Imm<1>{Common::Bit<3>(Vm)}).ZeroExtend() >> (esize == 16 ? 0 : 1);
    const auto scalar = ir.VectorGetElement(esize, ir.GetVector(m), index);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.VectorBroadcast(esize, scalar);
    const auto result = F ? ir.FPVectorMul(esize, reg_n, reg_m, false) : ir.VectorMultiply(esize, reg_n, reg_m);

    ir.SetVector(d, result);
    return true;
}

} // namespace Dynarmic::A32
