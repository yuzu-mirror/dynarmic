/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::asimd_VSWP(bool D, size_t Vd, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    // Swapping the same register results in the same contents.
    const auto d = ToExtRegD(Vd, D);
    const auto m = ToExtRegD(Vm, M);
    if (d == m) {
        return true;
    }

    const size_t regs = Q ? 2 : 1;
    for (size_t i = 0; i < regs; i++) {
        const auto d_index = d + i;
        const auto m_index = m + i;

        const auto reg_d = ir.GetExtendedRegister(d_index);
        const auto reg_m = ir.GetExtendedRegister(m_index);

        ir.SetExtendedRegister(m_index, reg_d);
        ir.SetExtendedRegister(d_index, reg_m);
    }

    return true;
}
} // namespace Dynarmic::A32
