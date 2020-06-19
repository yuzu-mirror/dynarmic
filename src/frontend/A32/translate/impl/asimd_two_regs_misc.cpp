/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

bool ArmTranslatorVisitor::asimd_VREV(bool D, size_t sz, size_t Vd, size_t op, bool Q, bool M, size_t Vm) {
    if (op + sz >= 3) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto result = [this, m, op, sz] {
        const auto reg_m = ir.GetVector(m);
        const size_t esize = 16U << sz;
        const auto shift = static_cast<u8>(8U << sz);

        // 64-bit regions
        if (op == 0b00) {
            IR::U128 result = ir.VectorOr(ir.VectorLogicalShiftRight(esize, reg_m, shift),
                                          ir.VectorLogicalShiftLeft(esize, reg_m, shift));

            switch (sz) {
            case 0: // 8-bit elements
                result = ir.VectorShuffleLowHalfwords(result, 0b00011011);
                result = ir.VectorShuffleHighHalfwords(result, 0b00011011);
                break;
            case 1: // 16-bit elements
                result = ir.VectorShuffleLowHalfwords(result, 0b01001110);
                result = ir.VectorShuffleHighHalfwords(result, 0b01001110);
                break;
            }

            return result;
        }

        // 32-bit regions
        if (op == 0b01) {
            IR::U128 result = ir.VectorOr(ir.VectorLogicalShiftRight(esize, reg_m, shift),
                                          ir.VectorLogicalShiftLeft(esize, reg_m, shift));

            // If dealing with 8-bit elements we'll need to shuffle the bytes in each halfword
            // e.g. Assume the following numbers point out bytes in a 32-bit word, we're essentially
            //      changing [3, 2, 1, 0] to [2, 3, 0, 1]
            if (sz == 0) {
                result = ir.VectorShuffleLowHalfwords(result, 0b10110001);
                result = ir.VectorShuffleHighHalfwords(result, 0b10110001);
            }

            return result;
        }

        // 16-bit regions
        return ir.VectorOr(ir.VectorLogicalShiftRight(esize, reg_m, 8),
                           ir.VectorLogicalShiftLeft(esize, reg_m, 8));
    }();
    
    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VCLS(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm) {
    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto result = [this, m, sz] {
        const auto reg_m = ir.GetVector(m);
        const size_t esize = 8U << sz;
        const auto shifted = ir.VectorArithmeticShiftRight(esize, reg_m, static_cast<u8>(esize));
        const auto xored = ir.VectorEor(reg_m, shifted);
        const auto clz = ir.VectorCountLeadingZeros(esize, xored);
        return ir.VectorSub(esize, clz, ir.VectorBroadcast(esize, I(esize, 1)));
    }();

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VCLZ(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm) {
    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto result = [this, m, sz] {
        const auto reg_m = ir.GetVector(m);
        const size_t esize = 8U << sz;

        return ir.VectorCountLeadingZeros(esize, reg_m);
    }();

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VCNT(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm) {
    if (sz != 0b00) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto reg_m = ir.GetVector(m);
    const auto result = ir.VectorPopulationCount(reg_m);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VMVN_reg(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm) {
    if (sz != 0b00) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = ir.GetVector(m);
    const auto result = ir.VectorNot(reg_m);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VQABS(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm) {
    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = ir.GetVector(m);
    const auto result = ir.VectorSignedSaturatedAbs(esize, reg_m);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VQNEG(bool D, size_t sz, size_t Vd, bool Q, bool M, size_t Vm) {
    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = ir.GetVector(m);
    const auto result = ir.VectorSignedSaturatedNeg(esize, reg_m);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VABS(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm) {
    if (sz == 0b11 || (F && sz != 0b10)) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto result = [this, F, m, sz] {
        const auto reg_m = ir.GetVector(m);

        if (F) {
            return ir.FPVectorAbs(32, reg_m);
        }

        const size_t esize = 8U << sz;
        return ir.VectorAbs(esize, reg_m);
    }();

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VNEG(bool D, size_t sz, size_t Vd, bool F, bool Q, bool M, size_t Vm) {
    if (sz == 0b11 || (F && sz != 0b10)) {
        return UndefinedInstruction();
    }
    
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto result = [this, F, m, sz] {
        const auto reg_m = ir.GetVector(m);

        if (F) {
            return ir.FPVectorNeg(32, reg_m);
        }

        const size_t esize = 8U << sz;
        return ir.VectorSub(esize, ir.ZeroVector(), reg_m);
    }();

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VSWP(bool D, size_t Vd, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    // Swapping the same register results in the same contents.
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    if (d == m) {
        return true;
    }

    if (Q) {
        const auto reg_d = ir.GetVector(d);
        const auto reg_m = ir.GetVector(m);

        ir.SetVector(m, reg_d);
        ir.SetVector(d, reg_m);
    } else {
        const auto reg_d = ir.GetExtendedRegister(d);
        const auto reg_m = ir.GetExtendedRegister(m);

        ir.SetExtendedRegister(m, reg_d);
        ir.SetExtendedRegister(d, reg_m);
    }

    return true;
}
} // namespace Dynarmic::A32
