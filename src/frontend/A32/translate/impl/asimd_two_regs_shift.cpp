/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {
namespace {
std::pair<size_t, size_t> ElementSizeAndShiftAmount(bool L, size_t imm6) {
    if (L) {
        return {64, 64U - imm6};
    }

    const int highest = Common::HighestSetBit(imm6 >> 3);
    if (highest == 0) {
        return {8, 16 - imm6};
    }

    if (highest == 1) {
        return {16, 32U - imm6};
    }

    return {32, 64U - imm6};
}
} // Anonymous namespace

bool ArmTranslatorVisitor::asimd_SHR(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    // Technically just a related encoding (One register and modified immediate instructions)
    if (!L && Common::Bits<3, 5>(imm6) == 0) {
        return UndefinedInstruction();
    }

    const auto [esize, shift_amount] = ElementSizeAndShiftAmount(L, imm6);
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = ir.GetVector(m);
    const auto result = U ? ir.VectorLogicalShiftRight(esize, reg_m, static_cast<u8>(shift_amount))
                          : ir.VectorArithmeticShiftRight(esize, reg_m, static_cast<u8>(shift_amount));

    ir.SetVector(d, result);
    return true;
}

} // namespace Dynarmic::A32
