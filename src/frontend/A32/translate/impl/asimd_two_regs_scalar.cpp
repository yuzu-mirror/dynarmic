/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/assert.h"
#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {
namespace {
enum class MultiplyBehavior {
    Multiply,
    MultiplyAccumulate,
    MultiplySubtract,
};

bool ScalarMultiply(ArmTranslatorVisitor& v, bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool F, bool N, bool M, size_t Vm,
                    MultiplyBehavior multiply) {
    ASSERT_MSG(sz != 0b11, "Decode error");
    if (sz == 0b00 || (F && sz == 0b01)) {
        return v.UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn))) {
        return v.UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto n = ToVector(Q, Vn, N);

    const auto m = ExtReg::Q0 + ((Vm >> 1) & (esize == 16 ? 0b11 : 0b111));
    const auto index = concatenate(Imm<1>{Common::Bit<0>(Vm)}, Imm<1>{M}, Imm<1>{Common::Bit<3>(Vm)}).ZeroExtend() >> (esize == 16 ? 0 : 1);
    const auto scalar = v.ir.VectorGetElement(esize, v.ir.GetVector(m), index);

    const auto reg_n = v.ir.GetVector(n);
    const auto reg_m = v.ir.VectorBroadcast(esize, scalar);
    const auto addend = F ? v.ir.FPVectorMul(esize, reg_n, reg_m, false)
                          : v.ir.VectorMultiply(esize, reg_n, reg_m);
    const auto result = [&] {
        switch (multiply) {
        case MultiplyBehavior::Multiply:
            return addend;
        case MultiplyBehavior::MultiplyAccumulate:
            return F ? v.ir.FPVectorAdd(esize, v.ir.GetVector(d), addend, false)
                     : v.ir.VectorAdd(esize, v.ir.GetVector(d), addend);
        case MultiplyBehavior::MultiplySubtract:
            return F ? v.ir.FPVectorSub(esize, v.ir.GetVector(d), addend, false)
                     : v.ir.VectorSub(esize, v.ir.GetVector(d), addend);
        default:
            return IR::U128{};
        }
    }();

    v.ir.SetVector(d, result);
    return true;
}
} // Anonymous namespace

bool ArmTranslatorVisitor::asimd_VMLA_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool F, bool N, bool M, size_t Vm) {
    const auto behavior = op ? MultiplyBehavior::MultiplySubtract
                             : MultiplyBehavior::MultiplyAccumulate;
    return ScalarMultiply(*this, Q, D, sz, Vn, Vd, F, N, M, Vm, behavior);
}

bool ArmTranslatorVisitor::asimd_VMUL_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool F, bool N, bool M, size_t Vm) {
    return ScalarMultiply(*this, Q, D, sz, Vn, Vd, F, N, M, Vm, MultiplyBehavior::Multiply);
}

} // namespace Dynarmic::A32
