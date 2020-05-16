/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_arm.h"

#include "common/bit_util.h"

namespace Dynarmic::A32 {

static ExtReg ToExtRegD(size_t base, bool bit) {
    return ExtReg::D0 + (base + (bit ? 16 : 0));
}

bool ArmTranslatorVisitor::v8_VLD_multiple(bool D, Reg n, size_t Vd, Imm<4> type, size_t size, size_t align, Reg m) {
    size_t nelem, regs, inc;
    switch (type.ZeroExtend()) {
    case 0b0111: // VLD1 A1
        nelem = 1;
        regs = 1;
        inc = 0;
        if (Common::Bit<1>(align)) {
            return UndefinedInstruction();
        }
        break;
    case 0b1010: // VLD1 A2
        nelem = 1;
        regs = 2;
        inc = 0;
        if (align == 0b11) {
            return UndefinedInstruction();
        }
        break;
    case 0b0110: // VLD1 A3
        nelem = 1;
        regs = 3;
        inc = 0;
        if (Common::Bit<1>(align)) {
            return UndefinedInstruction();
        }
        break;
    case 0b0010: // VLD1 A4
        nelem = 1;
        regs = 4;
        inc = 0;
        break;
    case 0b1000: // VLD2 A1
        nelem = 2;
        regs = 1;
        inc = 1;
        if (size == 0b11 || align == 0b11) {
            return UndefinedInstruction();
        }
        break;
    case 0b1001: // VLD2 A1
        nelem = 2;
        regs = 1;
        inc = 2;
        if (size == 0b11 || align == 0b11) {
            return UndefinedInstruction();
        }
        break;
    case 0b0011: // VLD2 A2
        nelem = 2;
        regs = 2;
        inc = 2;
        if (size == 0b11) {
            return UndefinedInstruction();
        }
        break;
    case 0b0100: // VLD3
        nelem = 3;
        regs = 1;
        inc = 1;
        if (size == 0b11 || Common::Bit<1>(align)) {
            return UndefinedInstruction();
        }
        break;
    case 0b0101: // VLD3
        nelem = 3;
        regs = 1;
        inc = 2;
        if (size == 0b11 || Common::Bit<1>(align)) {
            return UndefinedInstruction();
        }
        break;
    case 0b0000: // VLD4
        nelem = 4;
        regs = 1;
        inc = 1;
        if (size == 0b11) {
            return UndefinedInstruction();
        }
        break;
    case 0b0001: // VLD4
        nelem = 4;
        regs = 1;
        inc = 2;
        if (size == 0b11) {
            return UndefinedInstruction();
        }
        break;
    default:
        ASSERT_FALSE("Decode error");
    }

    const ExtReg d = ToExtRegD(Vd, D);
    const size_t d_last = RegNumber(d) + inc * (nelem - 1);
    if (n == Reg::R15 || d_last + regs > 32) {
        return UnpredictableInstruction();
    }

    [[maybe_unused]] const size_t alignment = align == 0 ? 1 : 4 << align;
    const size_t ebytes = static_cast<size_t>(1) << size;
    const size_t elements = 8 / ebytes;

    const bool wback = m != Reg::R15;
    const bool register_index = m != Reg::R15 && m != Reg::R13;

    for (size_t r = 0; r < regs; r++) {
        for (size_t i = 0; i < nelem; i++) {
            const ExtReg ext_reg = d + i * inc + r;
            ir.SetExtendedRegister(ext_reg, ir.Imm64(0));
        }
    }

    IR::U32 address = ir.GetRegister(n);
    for (size_t r = 0; r < regs; r++) {
        for (size_t e = 0; e < elements; e++) {
            for (size_t i = 0; i < nelem; i++) {
                const ExtReg ext_reg = d + i * inc + r;
                const IR::U64 element = ir.ZeroExtendToLong(ir.ReadMemory(ebytes * 8, address));
                const IR::U64 shifted_element = ir.LogicalShiftLeft(element, ir.Imm8(static_cast<u8>(e * ebytes * 8)));
                ir.SetExtendedRegister(ext_reg, ir.Or(ir.GetExtendedRegister(ext_reg), shifted_element));

                address = ir.Add(address, ir.Imm32(static_cast<u32>(ebytes)));
            }
        }
    }

    if (wback) {
        if (register_index) {
            ir.SetRegister(n, ir.Add(ir.GetRegister(n), ir.GetRegister(m)));
        } else {
            ir.SetRegister(n, ir.Add(ir.GetRegister(n), ir.Imm32(static_cast<u32>(8 * nelem * regs))));
        }
    }

    return true;
}

} // namespace Dynarmic::A32
