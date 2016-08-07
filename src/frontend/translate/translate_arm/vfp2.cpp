/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "translate_arm.h"

namespace Dynarmic {
namespace Arm {

static ExtReg ToExtReg(bool sz, size_t base, bool bit) {
    if (sz) {
        return static_cast<ExtReg>(static_cast<size_t>(ExtReg::D0) + base + (bit ? 16 : 0));
    } else {
        return static_cast<ExtReg>(static_cast<size_t>(ExtReg::S0) + (base << 1) + (bit ? 1 : 0));
    }
}

bool ArmTranslatorVisitor::vfp2_VADD(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg n = ToExtReg(sz, Vn, N);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VADD.{F32,F64} <{S,D}d>, <{S,D}n>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(n);
        auto b = ir.GetExtendedRegister(m);
        auto result = sz
                      ? ir.FPAdd64(a, b, true)
                      : ir.FPAdd32(a, b, true);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VABS(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VABS.{F32,F64} <{S,D}d>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(m);
        auto result = sz
                      ? ir.FPAbs64(a)
                      : ir.FPAbs32(a);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

} // namespace Arm
} // namespace Dynarmic
