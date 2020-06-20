/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/assert.h"
#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

static bool TableLookup(ArmTranslatorVisitor& v, bool is_vtbl, bool D, size_t Vn, size_t Vd, size_t len, bool N, bool M, size_t Vm) {
    const size_t length = len + 1;
    const auto d = ToVector(false, Vd, D);
    const auto m = ToVector(false, Vm, M);
    const auto n = ToVector(false, Vn, N);

    if (RegNumber(n) + length > 32) {
        return v.UnpredictableInstruction();
    }

    const IR::Table table = v.ir.VectorTable([&]{
        std::vector<IR::U64> result;
        for (size_t i = 0; i < length; ++i) {
            result.emplace_back(v.ir.GetExtendedRegister(n + i));
        }
        return result;
    }());
    const IR::U64 indicies = v.ir.GetExtendedRegister(m);
    const IR::U64 defaults = is_vtbl ? v.ir.Imm64(0) : IR::U64{v.ir.GetExtendedRegister(d)};
    const IR::U64 result = v.ir.VectorTableLookup(defaults, table, indicies);

    v.ir.SetExtendedRegister(d, result);
    return true;
}

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

bool ArmTranslatorVisitor::asimd_VTBL(bool D, size_t Vn, size_t Vd, size_t len, bool N, bool M, size_t Vm) {
    return TableLookup(*this, true, D, Vn, Vd, len, N, M, Vm);
}

bool ArmTranslatorVisitor::asimd_VTBX(bool D, size_t Vn, size_t Vd, size_t len, bool N, bool M, size_t Vm) {
    return TableLookup(*this, false, D, Vn, Vd, len, N, M, Vm);
}

} // namespace Dynarmic::A32
