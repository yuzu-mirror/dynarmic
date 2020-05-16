/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {
namespace {
ExtReg ToExtReg(size_t base, bool bit) {
    return ExtReg::D0 + (base + (bit ? 16 : 0));
}

template <typename Callable>
bool BitwiseInstruction(ArmTranslatorVisitor& v, bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm, Callable fn) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    const auto d = ToExtReg(Vd, D);
    const auto m = ToExtReg(Vm, M);
    const auto n = ToExtReg(Vn, N);
    const size_t regs = Q ? 2 : 1;

    for (size_t i = 0; i < regs; i++) {
        const IR::U32U64 reg_m = v.ir.GetExtendedRegister(m + i);
        const IR::U32U64 reg_n = v.ir.GetExtendedRegister(n + i);
        const IR::U32U64 result = fn(reg_n, reg_m);
        v.ir.SetExtendedRegister(d + i, result);
    }

    return true;
}

template <typename Callable>
bool BitwiseInstructionWithDst(ArmTranslatorVisitor& v, bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm, Callable fn) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    const auto d = ToExtReg(Vd, D);
    const auto m = ToExtReg(Vm, M);
    const auto n = ToExtReg(Vn, N);
    const size_t regs = Q ? 2 : 1;

    for (size_t i = 0; i < regs; i++) {
        const IR::U32U64 reg_d = v.ir.GetExtendedRegister(d + i);
        const IR::U32U64 reg_m = v.ir.GetExtendedRegister(m + i);
        const IR::U32U64 reg_n = v.ir.GetExtendedRegister(n + i);
        const IR::U32U64 result = fn(reg_d, reg_n, reg_m);
        v.ir.SetExtendedRegister(d + i, result);
    }

    return true;
}
} // Anonymous namespace

bool ArmTranslatorVisitor::asimd_VAND_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.And(reg_n, reg_m);
    });
}

bool ArmTranslatorVisitor::asimd_VBIC_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.And(reg_n, ir.Not(reg_m));
    });
}

bool ArmTranslatorVisitor::asimd_VORR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.Or(reg_n, reg_m);
    });
}

bool ArmTranslatorVisitor::asimd_VORN_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.Or(reg_n, ir.Not(reg_m));
    });
}

bool ArmTranslatorVisitor::asimd_VEOR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.Eor(reg_n, reg_m);
    });
}

bool ArmTranslatorVisitor::asimd_VBSL(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstructionWithDst(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.Or(ir.And(reg_n, reg_d), ir.And(reg_m, ir.Not(reg_d)));
    });
}

bool ArmTranslatorVisitor::asimd_VBIT(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstructionWithDst(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.Or(ir.And(reg_n, reg_m), ir.And(reg_d, ir.Not(reg_m)));
    });
}

bool ArmTranslatorVisitor::asimd_VBIF(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstructionWithDst(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.Or(ir.And(reg_d, reg_m), ir.And(reg_n, ir.Not(reg_m)));
    });
}
} // namespace Dynarmic::A32
