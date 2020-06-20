/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/assert.h"
#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {

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
    const size_t length = len + 1;
    const auto d = ToVector(false, Vd, D);
    const auto m = ToVector(false, Vm, M);
    const auto n = ToVector(false, Vn, N);

    if (RegNumber(n) + length > 32) {
        return UnpredictableInstruction();
    }

    const IR::U64 table0 = ir.GetExtendedRegister(n);
    const IR::U64 table1 = length >= 2 ? IR::U64{ir.GetExtendedRegister(n + 1)} : ir.Imm64(0);
    const IR::U64 table2 = length >= 3 ? IR::U64{ir.GetExtendedRegister(n + 2)} : ir.Imm64(0);
    const IR::U64 table3 = length == 4 ? IR::U64{ir.GetExtendedRegister(n + 3)} : ir.Imm64(0);

    const IR::Table table = ir.VectorTable(length <= 2
                                           ? std::vector<IR::U128>{ir.Pack2x64To1x128(table0, table1)}
                                           : std::vector<IR::U128>{ir.Pack2x64To1x128(table0, table1), ir.Pack2x64To1x128(table2, table3)});
    const IR::U128 indicies = ir.GetVector(m);
    const IR::U128 result = ir.VectorTableLookup(ir.ZeroVector(), table, indicies);

    ir.SetVector(d, result);
    return true;
}

} // namespace Dynarmic::A32
