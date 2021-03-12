/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"
#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_STMIA(bool W, Reg n, Imm<15> reg_list) {
    const auto regs_imm = reg_list.ZeroExtend();
    const auto num_regs = static_cast<u32>(Common::BitCount(regs_imm));

    if (n == Reg::PC || num_regs < 2) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), regs_imm)) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<13>()) {
        return UnpredictableInstruction();
    }

    IR::U32 address = ir.GetRegister(n);
    for (size_t i = 0; i < 15; i++) {
        if (Common::Bit(i, regs_imm)) {
            ir.WriteMemory32(address, ir.GetRegister(static_cast<Reg>(i)));
            address = ir.Add(address, ir.Imm32(4));
        }
    }

    if (W) {
        ir.SetRegister(n, address);
    }
    return true;
}

bool ThumbTranslatorVisitor::thumb32_STMDB(bool W, Reg n, Imm<15> reg_list) {
    const auto regs_imm = reg_list.ZeroExtend();
    const auto num_regs = static_cast<u32>(Common::BitCount(regs_imm));

    if (n == Reg::PC || num_regs < 2) {
        return UnpredictableInstruction();
    }
    if (W && Common::Bit(static_cast<size_t>(n), regs_imm)) {
        return UnpredictableInstruction();
    }
    if (reg_list.Bit<13>()) {
        return UnpredictableInstruction();
    }

    const IR::U32 start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(4 * num_regs));
    IR::U32 address = start_address;

    for (size_t i = 0; i < 15; i++) {
        if (Common::Bit(i, regs_imm)) {
            ir.WriteMemory32(address, ir.GetRegister(static_cast<Reg>(i)));
            address = ir.Add(address, ir.Imm32(4));
        }
    }

    if (W) {
        ir.SetRegister(n, start_address);
    }
    return true;
}

} // namespace Dynarmic::A32
