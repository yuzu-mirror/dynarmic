/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/assert.h"
#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {
namespace {
ExtReg ToExtRegD(size_t base, bool bit) {
    return ExtReg::D0 + (base + (bit ? 16 : 0));
}

u64 AdvSIMDExpandImm(bool op, Imm<4> cmode, Imm<8> imm8) {
    switch (cmode.Bits<1, 3>()) {
    case 0b000:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>(), 32);
    case 0b001:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>() << 8, 32);
    case 0b010:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>() << 16, 32);
    case 0b011:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>() << 24, 32);
    case 0b100:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>(), 16);
    case 0b101:
        return Common::Replicate<u64>(imm8.ZeroExtend<u64>() << 8, 16);
    case 0b110:
        if (!cmode.Bit<0>()) {
            return Common::Replicate<u64>((imm8.ZeroExtend<u64>() << 8) | Common::Ones<u64>(8), 32);
        }
        return Common::Replicate<u64>((imm8.ZeroExtend<u64>() << 16) | Common::Ones<u64>(16), 32);
    case 0b111:
        if (!cmode.Bit<0>() && !op) {
            return Common::Replicate<u64>(imm8.ZeroExtend<u64>(), 8);
        }
        if (!cmode.Bit<0>() && op) {
            u64 result = 0;
            result |= imm8.Bit<0>() ? Common::Ones<u64>(8) << (0 * 8) : 0;
            result |= imm8.Bit<1>() ? Common::Ones<u64>(8) << (1 * 8) : 0;
            result |= imm8.Bit<2>() ? Common::Ones<u64>(8) << (2 * 8) : 0;
            result |= imm8.Bit<3>() ? Common::Ones<u64>(8) << (3 * 8) : 0;
            result |= imm8.Bit<4>() ? Common::Ones<u64>(8) << (4 * 8) : 0;
            result |= imm8.Bit<5>() ? Common::Ones<u64>(8) << (5 * 8) : 0;
            result |= imm8.Bit<6>() ? Common::Ones<u64>(8) << (6 * 8) : 0;
            result |= imm8.Bit<7>() ? Common::Ones<u64>(8) << (7 * 8) : 0;
            return result;
        }
        if (cmode.Bit<0>() && !op) {
            u64 result = 0;
            result |= imm8.Bit<7>() ? 0x80000000 : 0;
            result |= imm8.Bit<6>() ? 0x3E000000 : 0x40000000;
            result |= imm8.Bits<0, 5, u64>() << 19;
            return Common::Replicate<u64>(result, 32);
        }
        if (cmode.Bit<0>() && op) {
            u64 result = 0;
            result |= imm8.Bit<7>() ? 0x80000000'00000000 : 0;
            result |= imm8.Bit<6>() ? 0x3FC00000'00000000 : 0x40000000'00000000;
            result |= imm8.Bits<0, 5, u64>() << 48;
            return result;
        }
    }
    UNREACHABLE();
}
} // Anonymous namespace

bool ArmTranslatorVisitor::asimd_VMOV_imm(Imm<1> a, bool D, Imm<1> b, Imm<1> c, Imm<1> d, size_t Vd,
                                          Imm<4> cmode, bool Q, bool op, Imm<1> e, Imm<1> f, Imm<1> g, Imm<1> h) {
    if (Q && Common::Bit<0>(Vd)) {
        return UndefinedInstruction();
    }

    const auto d_reg = ToExtRegD(Vd, D);
    const size_t regs = Q ? 2 : 1;
    const auto imm = AdvSIMDExpandImm(op, cmode, concatenate(a, b, c, d, e, f, g, h));

    // VMOV
    const auto mov = [&] {
        const auto imm64 = ir.Imm64(imm);
        for (size_t i = 0; i < regs; i++) {
            ir.SetExtendedRegister(d_reg + i, imm64);
        }
        return true;
    };

    // VMVN
    const auto mvn = [&] {
        const auto imm64 = ir.Imm64(~imm);
        for (size_t i = 0; i < regs; i++) {
            ir.SetExtendedRegister(d_reg + i, imm64);
        }
        return true;
    };

    // VORR
    const auto orr = [&] {
        const auto imm64 = ir.Imm64(imm);
        for (size_t i = 0; i < regs; i++) {
            const auto d_index = d_reg + i;
            const auto reg_value = ir.GetExtendedRegister(d_index);
            ir.SetExtendedRegister(d_index, ir.Or(reg_value, imm64));
        }
        return true;
    };

    // VBIC
    const auto bic = [&] {
        const auto imm64 = ir.Imm64(~imm);
        for (size_t i = 0; i < regs; i++) {
            const auto d_index = d_reg + i;
            const auto reg_value = ir.GetExtendedRegister(d_index);
            ir.SetExtendedRegister(d_index, ir.And(reg_value, imm64));
        }
        return true;
    };

    switch (concatenate(cmode, Imm<1>{op}).ZeroExtend()) {
    case 0b00000: case 0b00100:
    case 0b01000: case 0b01100:
    case 0b10000: case 0b10100:
    case 0b11000: case 0b11010:
    case 0b11100: case 0b11101:
    case 0b11110:
        return mov();
    case 0b11111:
        return UndefinedInstruction();
    case 0b00001: case 0b00101:
    case 0b01001: case 0b01101:
    case 0b10001: case 0b10101:
    case 0b11001: case 0b11011:
        return mvn();
    case 0b00010: case 0b00110:
    case 0b01010: case 0b01110:
    case 0b10010: case 0b10110:
        return orr();
    case 0b00011: case 0b00111:
    case 0b01011: case 0b01111:
    case 0b10011: case 0b10111:
        return bic();
    }

    UNREACHABLE();
}

} // namespace Dynarmic::A32
