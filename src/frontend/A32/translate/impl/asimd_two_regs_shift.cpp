/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/assert.h"
#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {
namespace {
enum class Accumulating {
    None,
    Accumulate
};

enum class Rounding {
    None,
    Round,
};

enum class Narrowing {
    Truncation,
    SaturateToUnsigned,
    SaturateToSigned,
};

enum class Signedness {
    Signed,
    Unsigned
};

IR::U128 PerformRoundingCorrection(ArmTranslatorVisitor& v, size_t esize, u64 round_value, IR::U128 original, IR::U128 shifted) {
    const auto round_const = v.ir.VectorBroadcast(esize, v.I(esize, round_value));
    const auto round_correction = v.ir.VectorEqual(esize, v.ir.VectorAnd(original, round_const), round_const);
    return v.ir.VectorSub(esize, shifted, round_correction);
}

std::pair<size_t, size_t> ElementSizeAndShiftAmount(bool right_shift, bool L, size_t imm6) {
    if (right_shift) {
        if (L) {
            return {64, 64 - imm6};
        }

        const size_t esize = 8U << Common::HighestSetBit(imm6 >> 3);
        const size_t shift_amount = (esize * 2) - imm6;
        return {esize, shift_amount};
    } else {
        if (L) {
            return {64, imm6};
        }

        const size_t esize = 8U << Common::HighestSetBit(imm6 >> 3);
        const size_t shift_amount = imm6 - esize;
        return {esize, shift_amount};
    }
}

bool ShiftRight(ArmTranslatorVisitor& v, bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm,
                Accumulating accumulate, Rounding rounding) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    // Technically just a related encoding (One register and modified immediate instructions)
    if (!L && Common::Bits<3, 5>(imm6) == 0) {
        ASSERT_FALSE();
    }

    const auto [esize, shift_amount] = ElementSizeAndShiftAmount(true, L, imm6);
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = v.ir.GetVector(m);
    auto result = U ? v.ir.VectorLogicalShiftRight(esize, reg_m, static_cast<u8>(shift_amount))
                    : v.ir.VectorArithmeticShiftRight(esize, reg_m, static_cast<u8>(shift_amount));

    if (rounding == Rounding::Round) {
        const u64 round_value = 1ULL << (shift_amount - 1);
        result = PerformRoundingCorrection(v, esize, round_value, reg_m, result);
    }

    if (accumulate == Accumulating::Accumulate) {
        const auto reg_d = v.ir.GetVector(d);
        result = v.ir.VectorAdd(esize, result, reg_d);
    }

    v.ir.SetVector(d, result);
    return true;
}

bool ShiftRightNarrowing(ArmTranslatorVisitor& v, bool D, size_t imm6, size_t Vd, bool M, size_t Vm,
                         Rounding rounding, Narrowing narrowing, Signedness signedness) {
    if (Common::Bits<3, 5>(imm6) == 0) {
        // TODO: Decode error
        return v.UndefinedInstruction();
    }

    if (Common::Bit<0>(Vm)) {
        return v.UndefinedInstruction();
    }

    const auto [esize, shift_amount_] = ElementSizeAndShiftAmount(true, false, imm6);
    const auto source_esize = 2 * esize;
    const auto shift_amount = static_cast<u8>(shift_amount_);

    const auto d = ToVector(false, Vd, D);
    const auto m = ToVector(true, Vm, M);

    const auto reg_m = v.ir.GetVector(m);
    auto wide_result = [&] {
        if (signedness == Signedness::Signed) {
            return v.ir.VectorArithmeticShiftRight(source_esize, reg_m, shift_amount);
        }
        return v.ir.VectorLogicalShiftRight(source_esize, reg_m, shift_amount);
    }();

    if (rounding == Rounding::Round) {
        const u64 round_value = 1ULL << (shift_amount - 1);
        wide_result = PerformRoundingCorrection(v, source_esize, round_value, reg_m, wide_result);
    }

    const auto result = [&] {
        switch (narrowing) {
        case Narrowing::Truncation:
            return v.ir.VectorNarrow(source_esize, wide_result);
        case Narrowing::SaturateToUnsigned:
            if (signedness == Signedness::Signed) {
                return v.ir.VectorSignedSaturatedNarrowToUnsigned(source_esize, wide_result);
            }
            return v.ir.VectorUnsignedSaturatedNarrow(source_esize, wide_result);
        case Narrowing::SaturateToSigned:
            ASSERT(signedness == Signedness::Signed);
            return v.ir.VectorSignedSaturatedNarrowToSigned(source_esize, wide_result);
        }
        UNREACHABLE();
    }();

    v.ir.SetVector(d, result);
    return true;
}
} // Anonymous namespace

bool ArmTranslatorVisitor::asimd_SHR(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    return ShiftRight(*this, U, D, imm6, Vd, L, Q, M, Vm,
                      Accumulating::None, Rounding::None);
}

bool ArmTranslatorVisitor::asimd_SRA(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    return ShiftRight(*this, U, D, imm6, Vd, L, Q, M, Vm,
                      Accumulating::Accumulate, Rounding::None);
}

bool ArmTranslatorVisitor::asimd_VRSHR(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    return ShiftRight(*this, U, D, imm6, Vd, L, Q, M, Vm,
                      Accumulating::None, Rounding::Round);
}

bool ArmTranslatorVisitor::asimd_VRSRA(bool U, bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    return ShiftRight(*this, U, D, imm6, Vd, L, Q, M, Vm,
                      Accumulating::Accumulate, Rounding::Round);
}

bool ArmTranslatorVisitor::asimd_VSRI(bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    // Technically just a related encoding (One register and modified immediate instructions)
    if (!L && Common::Bits<3, 5>(imm6) == 0) {
        ASSERT_FALSE();
    }

    const auto [esize, shift_amount] = ElementSizeAndShiftAmount(true, L, imm6);
    const u64 mask = shift_amount == esize ? 0 : Common::Ones<u64>(esize) >> shift_amount;

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = ir.GetVector(m);
    const auto reg_d = ir.GetVector(d);

    const auto shifted = ir.VectorLogicalShiftRight(esize, reg_m, static_cast<u8>(shift_amount));
    const auto mask_vec = ir.VectorBroadcast(esize, I(esize, mask));
    const auto result = ir.VectorOr(ir.VectorAnd(reg_d, ir.VectorNot(mask_vec)), shifted);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VSLI(bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    // Technically just a related encoding (One register and modified immediate instructions)
    if (!L && Common::Bits<3, 5>(imm6) == 0) {
        return UndefinedInstruction();
    }

    const auto [esize, shift_amount] = ElementSizeAndShiftAmount(false, L, imm6);
    const u64 mask = Common::Ones<u64>(esize) << shift_amount;

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = ir.GetVector(m);
    const auto reg_d = ir.GetVector(d);

    const auto shifted = ir.VectorLogicalShiftLeft(esize, reg_m, static_cast<u8>(shift_amount));
    const auto mask_vec = ir.VectorBroadcast(esize, I(esize, mask));
    const auto result = ir.VectorOr(ir.VectorAnd(reg_d, ir.VectorNot(mask_vec)), shifted);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VQSHL(bool U, bool D, size_t imm6, size_t Vd, bool op, bool L, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (!U && !op) {
        return UndefinedInstruction();
    }

    // Technically just a related encoding (One register and modified immediate instructions)
    if (!L && Common::Bits<3, 5>(imm6) == 0) {
        ASSERT_FALSE();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto result = [&] {
        const auto reg_m = ir.GetVector(m);
        const auto [esize, shift_amount] = ElementSizeAndShiftAmount(false, L, imm6);
        const IR::U128 shift_vec = ir.VectorBroadcast(esize, I(esize, shift_amount));

        if (U) {
            if (op) {
                return ir.VectorUnsignedSaturatedShiftLeft(esize, reg_m, shift_vec);
            }

            return ir.VectorSignedSaturatedShiftLeftUnsigned(esize, reg_m, shift_vec);
        }
        if (op) {
            return ir.VectorSignedSaturatedShiftLeft(esize, reg_m, shift_vec);
        }

        return IR::U128{};
    }();

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VSHL(bool D, size_t imm6, size_t Vd, bool L, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    // Technically just a related encoding (One register and modified immediate instructions)
    if (!L && Common::Bits<3, 5>(imm6) == 0) {
        ASSERT_FALSE();
    }

    const auto [esize, shift_amount] = ElementSizeAndShiftAmount(false, L, imm6);
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = ir.GetVector(m);
    const auto result = ir.VectorLogicalShiftLeft(esize, reg_m, static_cast<u8>(shift_amount));

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VSHRN(bool D, size_t imm6, size_t Vd, bool M, size_t Vm) {
    return ShiftRightNarrowing(*this, D, imm6, Vd, M, Vm,
                               Rounding::None, Narrowing::Truncation, Signedness::Unsigned);
}

bool ArmTranslatorVisitor::asimd_VRSHRN(bool D, size_t imm6, size_t Vd, bool M, size_t Vm) {
    return ShiftRightNarrowing(*this, D, imm6, Vd, M, Vm,
                               Rounding::Round, Narrowing::Truncation, Signedness::Unsigned);
}

bool ArmTranslatorVisitor::asimd_VQRSHRUN(bool D, size_t imm6, size_t Vd, bool M, size_t Vm) {
    return ShiftRightNarrowing(*this, D, imm6, Vd, M, Vm,
                               Rounding::Round, Narrowing::SaturateToUnsigned, Signedness::Signed);
}

bool ArmTranslatorVisitor::asimd_VQSHRUN(bool D, size_t imm6, size_t Vd, bool M, size_t Vm) {
    return ShiftRightNarrowing(*this, D, imm6, Vd, M, Vm,
                               Rounding::None, Narrowing::SaturateToUnsigned, Signedness::Signed);
}

bool ArmTranslatorVisitor::asimd_VQSHRN(bool U, bool D, size_t imm6, size_t Vd, bool M, size_t Vm) {
    return ShiftRightNarrowing(*this, D, imm6, Vd, M, Vm,
                               Rounding::None, U ? Narrowing::SaturateToUnsigned : Narrowing::SaturateToSigned, U ? Signedness::Unsigned : Signedness::Signed);
}

bool ArmTranslatorVisitor::asimd_VQRSHRN(bool U, bool D, size_t imm6, size_t Vd, bool M, size_t Vm) {
    return ShiftRightNarrowing(*this, D, imm6, Vd, M, Vm,
                               Rounding::Round, U ? Narrowing::SaturateToUnsigned : Narrowing::SaturateToSigned, U ? Signedness::Unsigned : Signedness::Signed);
}

bool ArmTranslatorVisitor::asimd_VCVT_fixed(bool U, bool D, size_t imm6, size_t Vd, bool to_fixed, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    ASSERT_MSG((Common::Bits<3, 5>(imm6) != 0), "Decode error");

    if (!Common::Bit<5>(imm6)) {
        return UndefinedInstruction();
    }

    const size_t fbits = 64 - imm6;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);

    const auto reg_m = ir.GetVector(m);
    const auto result = to_fixed ? (U ? ir.FPVectorToUnsignedFixed(32, reg_m, fbits, FP::RoundingMode::TowardsZero, false) : ir.FPVectorToSignedFixed(32, reg_m, fbits, FP::RoundingMode::TowardsZero, false))
                                 : (U ? ir.FPVectorFromUnsignedFixed(32, reg_m, fbits, FP::RoundingMode::ToNearest_TieEven, false) : ir.FPVectorFromSignedFixed(32, reg_m, fbits, FP::RoundingMode::ToNearest_TieEven, false));

    ir.SetVector(d, result);
    return true;
}

} // namespace Dynarmic::A32
