/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {
static ExtReg ToExtReg(size_t base, bool bit) {
    return ExtReg::D0 + (base + (bit ? 16 : 0));
}

bool ArmTranslatorVisitor::asimd_VAND_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToExtReg(Vd, D);
    const auto m = ToExtReg(Vm, M);
    const auto n = ToExtReg(Vn, N);
    const size_t regs = Q ? 2 : 1;

    for (size_t i = 0; i < regs; i++) {
        const IR::U32U64 reg_m = ir.GetExtendedRegister(m + i);
        const IR::U32U64 reg_n = ir.GetExtendedRegister(n + i);
        const IR::U32U64 result = ir.And(reg_n, reg_m);
        ir.SetExtendedRegister(d + i, result);
    }

    return true;
}

bool ArmTranslatorVisitor::asimd_VBIC_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToExtReg(Vd, D);
    const auto m = ToExtReg(Vm, M);
    const auto n = ToExtReg(Vn, N);
    const size_t regs = Q ? 2 : 1;

    for (size_t i = 0; i < regs; i++) {
        const IR::U32U64 reg_m = ir.GetExtendedRegister(m + i);
        const IR::U32U64 reg_n = ir.GetExtendedRegister(n + i);
        const IR::U32U64 result = ir.And(reg_n, ir.Not(reg_m));
        ir.SetExtendedRegister(d + i, result);
    }

    return true;
}

bool ArmTranslatorVisitor::asimd_VORR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToExtReg(Vd, D);
    const auto m = ToExtReg(Vm, M);
    const auto n = ToExtReg(Vn, N);
    const size_t regs = Q ? 2 : 1;

    for (size_t i = 0; i < regs; i++) {
        const IR::U32U64 reg_m = ir.GetExtendedRegister(m + i);
        const IR::U32U64 reg_n = ir.GetExtendedRegister(n + i);
        const IR::U32U64 result = ir.Or(reg_n, reg_m);
        ir.SetExtendedRegister(d + i, result);
    }

    return true;
}

bool ArmTranslatorVisitor::asimd_VORN_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToExtReg(Vd, D);
    const auto m = ToExtReg(Vm, M);
    const auto n = ToExtReg(Vn, N);
    const size_t regs = Q ? 2 : 1;

    for (size_t i = 0; i < regs; i++) {
        const IR::U32U64 reg_m = ir.GetExtendedRegister(m + i);
        const IR::U32U64 reg_n = ir.GetExtendedRegister(n + i);
        const IR::U32U64 result = ir.Or(reg_n, ir.Not(reg_m));
        ir.SetExtendedRegister(d + i, result);
    }

    return true;
}

bool ArmTranslatorVisitor::asimd_VEOR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToExtReg(Vd, D);
    const auto m = ToExtReg(Vm, M);
    const auto n = ToExtReg(Vn, N);
    const size_t regs = Q ? 2 : 1;

    for (size_t i = 0; i < regs; i++) {
        const IR::U32U64 reg_m = ir.GetExtendedRegister(m + i);
        const IR::U32U64 reg_n = ir.GetExtendedRegister(n + i);
        const IR::U32U64 result = ir.Eor(reg_n, reg_m);
        ir.SetExtendedRegister(d + i, result);
    }

    return true;
}

} // namespace Dynarmic::A32
