/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {
namespace {
enum class Accumulating {
    None,
    Accumulate
};

std::pair<size_t, size_t> ElementSizeAndShiftAmount(bool L, size_t imm6) {
    if (L) {
        return {64, 64 - imm6};
    }

    const size_t esize = 8U << Common::HighestSetBit(imm6 >> 3);
    const size_t shift_amount = (esize * 2) - imm6;
    return {esize, shift_amount};
}

bool ShiftRight(ArmTranslatorVisitor& v, bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm,
                Accumulating accumulate) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    // Technically just a related encoding (One register and modified immediate instructions)
    if (!L && Common::Bits<3, 5>(imm6) == 0) {
        return v.UndefinedInstruction();
    }

    const auto [esize, shift_amount] = ElementSizeAndShiftAmount(L, imm6);
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = v.ir.GetVector(m);
    auto result = U ? v.ir.VectorLogicalShiftRight(esize, reg_m, static_cast<u8>(shift_amount))
                      : v.ir.VectorArithmeticShiftRight(esize, reg_m, static_cast<u8>(shift_amount));

    if (accumulate == Accumulating::Accumulate) {
        const auto reg_d = v.ir.GetVector(d);
        result = v.ir.VectorAdd(esize, result, reg_d);
    }

    v.ir.SetVector(d, result);
    return true;
}
} // Anonymous namespace

bool ArmTranslatorVisitor::asimd_SHR(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    return ShiftRight(*this, U, D, imm6, Vd, L, Q, M, Vm, Accumulating::None);
}

bool ArmTranslatorVisitor::asimd_SRA(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    return ShiftRight(*this, U, D, imm6, Vd, L, Q, M, Vm, Accumulating::Accumulate);
}

} // namespace Dynarmic::A32
