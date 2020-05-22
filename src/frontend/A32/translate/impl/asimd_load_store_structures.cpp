/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_arm.h"

#include <optional>
#include <tuple>
#include "common/bit_util.h"

namespace Dynarmic::A32 {

namespace {

std::optional<std::tuple<size_t, size_t, size_t>> DecodeType(Imm<4> type, size_t size, size_t align) {
    switch (type.ZeroExtend()) {
    case 0b0111: // VST1 A1 / VLD1 A1
        if (Common::Bit<1>(align)) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{1, 1, 0};
    case 0b1010: // VST1 A2 / VLD1 A2
        if (align == 0b11) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{1, 2, 0};
    case 0b0110: // VST1 A3 / VLD1 A3
        if (Common::Bit<1>(align)) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{1, 3, 0};
    case 0b0010: // VST1 A4 / VLD1 A4
        return std::tuple<size_t, size_t, size_t>{1, 4, 0};
    case 0b1000: // VST2 A1 / VLD2 A1
        if (size == 0b11 || align == 0b11) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{2, 1, 1};
    case 0b1001: // VST2 A1 / VLD2 A1
        if (size == 0b11 || align == 0b11) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{2, 1, 2};
    case 0b0011: // VST2 A2 / VLD2 A2
        if (size == 0b11) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{2, 2, 2};
    case 0b0100: // VST3 / VLD3
        if (size == 0b11 || Common::Bit<1>(align)) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{3, 1, 1};
    case 0b0101: // VST3 / VLD3
        if (size == 0b11 || Common::Bit<1>(align)) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{3, 1, 2};
    case 0b0000: // VST4 / VLD4
        if (size == 0b11) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{4, 1, 1};
    case 0b0001: // VST4 / VLD4
        if (size == 0b11) {
            return std::nullopt;
        }
        return std::tuple<size_t, size_t, size_t>{4, 1, 2};
    }
    ASSERT_FALSE("Decode error");
}
} // anoynmous namespace

bool ArmTranslatorVisitor::v8_VST_multiple(bool D, Reg n, size_t Vd, Imm<4> type, size_t size, size_t align, Reg m) {
    const auto decoded_type = DecodeType(type, size, align);
    if (!decoded_type) {
        return UndefinedInstruction();
    }
    const auto [nelem, regs, inc] = *decoded_type;

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

    IR::U32 address = ir.GetRegister(n);
    for (size_t r = 0; r < regs; r++) {
        for (size_t e = 0; e < elements; e++) {
            for (size_t i = 0; i < nelem; i++) {
                const ExtReg ext_reg = d + i * inc + r;
                const IR::U64 shifted_element = ir.LogicalShiftRight(ir.GetExtendedRegister(ext_reg), ir.Imm8(static_cast<u8>(e * ebytes * 8)));
                const IR::UAny element = ir.LeastSignificant(8 * ebytes, shifted_element);
                ir.WriteMemory(8 * ebytes, address, element);

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

bool ArmTranslatorVisitor::v8_VLD_multiple(bool D, Reg n, size_t Vd, Imm<4> type, size_t size, size_t align, Reg m) {
    const auto decoded_type = DecodeType(type, size, align);
    if (!decoded_type) {
        return UndefinedInstruction();
    }
    const auto [nelem, regs, inc] = *decoded_type;

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
