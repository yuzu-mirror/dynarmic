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

bool ArmTranslatorVisitor::vfp2_VSUB(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg n = ToExtReg(sz, Vn, N);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VSUB.{F32,F64} <{S,D}d>, <{S,D}n>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(n);
        auto b = ir.GetExtendedRegister(m);
        auto result = sz
                      ? ir.FPSub64(a, b, true)
                      : ir.FPSub32(a, b, true);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg n = ToExtReg(sz, Vn, N);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VMUL.{F32,F64} <{S,D}d>, <{S,D}n>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(n);
        auto b = ir.GetExtendedRegister(m);
        auto result = sz
                      ? ir.FPMul64(a, b, true)
                      : ir.FPMul32(a, b, true);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg n = ToExtReg(sz, Vn, N);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VMLA.{F32,F64} <{S,D}d>, <{S,D}n>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(n);
        auto b = ir.GetExtendedRegister(m);
        auto c = ir.GetExtendedRegister(d);
        auto result = sz
                      ? ir.FPAdd64(c, ir.FPMul64(a, b, true), true)
                      : ir.FPAdd32(c, ir.FPMul32(a, b, true), true);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg n = ToExtReg(sz, Vn, N);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VMLS.{F32,F64} <{S,D}d>, <{S,D}n>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(n);
        auto b = ir.GetExtendedRegister(m);
        auto c = ir.GetExtendedRegister(d);
        auto result = sz
                      ? ir.FPAdd64(c, ir.FPNeg64(ir.FPMul64(a, b, true)), true)
                      : ir.FPAdd32(c, ir.FPNeg32(ir.FPMul32(a, b, true)), true);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VNMUL(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg n = ToExtReg(sz, Vn, N);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VNMUL.{F32,F64} <{S,D}d>, <{S,D}n>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(n);
        auto b = ir.GetExtendedRegister(m);
        auto result = sz
                      ? ir.FPNeg64(ir.FPMul64(a, b, true))
                      : ir.FPNeg32(ir.FPMul32(a, b, true));
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VNMLA(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg n = ToExtReg(sz, Vn, N);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VNMLA.{F32,F64} <{S,D}d>, <{S,D}n>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(n);
        auto b = ir.GetExtendedRegister(m);
        auto c = ir.GetExtendedRegister(d);
        auto result = sz
                      ? ir.FPAdd64(ir.FPNeg64(c), ir.FPNeg64(ir.FPMul64(a, b, true)), true)
                      : ir.FPAdd32(ir.FPNeg32(c), ir.FPNeg32(ir.FPMul32(a, b, true)), true);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VNMLS(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg n = ToExtReg(sz, Vn, N);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VNMLS.{F32,F64} <{S,D}d>, <{S,D}n>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(n);
        auto b = ir.GetExtendedRegister(m);
        auto c = ir.GetExtendedRegister(d);
        auto result = sz
                      ? ir.FPAdd64(ir.FPNeg64(c), ir.FPMul64(a, b, true), true)
                      : ir.FPAdd32(ir.FPNeg32(c), ir.FPMul32(a, b, true), true);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VDIV(Cond cond, bool D, size_t Vn, size_t Vd, bool sz, bool N, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg n = ToExtReg(sz, Vn, N);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VDIV.{F32,F64} <{S,D}d>, <{S,D}n>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(n);
        auto b = ir.GetExtendedRegister(m);
        auto result = sz
                      ? ir.FPDiv64(a, b, true)
                      : ir.FPDiv32(a, b, true);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMOV_u32_f64(Cond cond, size_t Vd, Reg t, bool D) {
    ExtReg d = ToExtReg(true, Vd, D);
    if (t == Reg::PC)
        return UnpredictableInstruction();
    // VMOV.32 <Dd[0]>, <Rt>
    if (ConditionPassed(cond)) {
        auto d_f64 = ir.GetExtendedRegister(d);
        auto t_u32 = ir.GetRegister(t);

        auto d_u64 = ir.TransferFromFP64(d_f64);
        auto result = ir.Pack2x32To1x64(t_u32, ir.MostSignificantWord(d_u64).result);

        ir.SetExtendedRegister(d, ir.TransferToFP64(result));
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMOV_f64_u32(Cond cond, size_t Vn, Reg t, bool N) {
    ExtReg n = ToExtReg(true, Vn, N);
    if (t == Reg::PC)
        return UnpredictableInstruction();
    // VMOV.32 <Rt>, <Dn[0]>
    if (ConditionPassed(cond)) {
        auto n_f64 = ir.GetExtendedRegister(n);
        ir.SetRegister(t, ir.LeastSignificantWord(ir.TransferFromFP64(n_f64)));
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMOV_u32_f32(Cond cond, size_t Vn, Reg t, bool N) {
    ExtReg n = ToExtReg(false, Vn, N);
    if (t == Reg::PC)
        return UnpredictableInstruction();
    // VMOV <Sn>, <Rt>
    if (ConditionPassed(cond)) {
        ir.SetExtendedRegister(n, ir.TransferToFP32(ir.GetRegister(t)));
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMOV_f32_u32(Cond cond, size_t Vn, Reg t, bool N) {
    ExtReg n = ToExtReg(false, Vn, N);
    if (t == Reg::PC)
        return UnpredictableInstruction();
    // VMOV <Rt>, <Sn>
    if (ConditionPassed(cond)) {
        ir.SetRegister(t, ir.TransferFromFP32(ir.GetExtendedRegister(n)));
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMOV_2u32_2f32(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
    ExtReg m = ToExtReg(false, Vm, M);
    if (t == Reg::PC || t2 == Reg::PC || m == ExtReg::S31)
        return UnpredictableInstruction();
    // VMOV <Sm>, <Sm1>, <Rt>, <Rt2>
    if (ConditionPassed(cond)) {
        ir.SetExtendedRegister(m, ir.TransferToFP32(ir.GetRegister(t)));
        ir.SetExtendedRegister(m+1, ir.TransferToFP32(ir.GetRegister(t2)));
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMOV_2f32_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
    ExtReg m = ToExtReg(false, Vm, M);
    if (t == Reg::PC || t2 == Reg::PC || m == ExtReg::S31)
        return UnpredictableInstruction();
    if (t == t2)
        return UnpredictableInstruction();
    // VMOV <Rt>, <Rt2>, <Sm>, <Sm1>
    if (ConditionPassed(cond)) {
        ir.SetRegister(t, ir.TransferFromFP32(ir.GetExtendedRegister(m)));
        ir.SetRegister(t2, ir.TransferFromFP32(ir.GetExtendedRegister(m+1)));
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMOV_2u32_f64(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
    ExtReg m = ToExtReg(true, Vm, M);
    if (t == Reg::PC || t2 == Reg::PC || m == ExtReg::S31)
        return UnpredictableInstruction();
    // VMOV<c> <Dm>, <Rt>, <Rt2>
    if (ConditionPassed(cond)) {
        auto value = ir.Pack2x32To1x64(ir.GetRegister(t), ir.GetRegister(t2));
        ir.SetExtendedRegister(m, ir.TransferToFP64(value));
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMOV_f64_2u32(Cond cond, Reg t2, Reg t, bool M, size_t Vm) {
    ExtReg m = ToExtReg(true, Vm, M);
    if (t == Reg::PC || t2 == Reg::PC || m == ExtReg::S31)
        return UnpredictableInstruction();
    if (t == t2)
        return UnpredictableInstruction();
    // VMOV<c> <Rt>, <Rt2>, <Dm>
    if (ConditionPassed(cond)) {
        auto value = ir.TransferFromFP64(ir.GetExtendedRegister(m));
        ir.SetRegister(t, ir.LeastSignificantWord(value));
        ir.SetRegister(t2, ir.MostSignificantWord(value).result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VMOV_reg(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VMOV.{F32,F64} <{S,D}d>, <{S,D}m>
    if (ConditionPassed(cond)) {
        ir.SetExtendedRegister(d, ir.GetExtendedRegister(m));
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

bool ArmTranslatorVisitor::vfp2_VNEG(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VNEG.{F32,F64} <{S,D}d>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(m);
        auto result = sz
                      ? ir.FPNeg64(a)
                      : ir.FPNeg32(a);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VSQRT(Cond cond, bool D, size_t Vd, bool sz, bool M, size_t Vm) {
    if (ir.current_location.FPSCR_Len() != 1 || ir.current_location.FPSCR_Stride() != 1)
        return InterpretThisInstruction(); // TODO: Vectorised floating point instructions

    ExtReg d = ToExtReg(sz, Vd, D);
    ExtReg m = ToExtReg(sz, Vm, M);
    // VSQRT.{F32,F64} <{S,D}d>, <{S,D}m>
    if (ConditionPassed(cond)) {
        auto a = ir.GetExtendedRegister(m);
        auto result = sz
                      ? ir.FPSqrt64(a)
                      : ir.FPSqrt32(a);
        ir.SetExtendedRegister(d, result);
    }
    return true;
}

bool ArmTranslatorVisitor::vfp2_VLDR(Cond cond, bool U, bool D, Reg n, size_t Vd, bool sz, Imm8 imm8) {
    u32 imm32 = imm8 << 2;
    ExtReg d = ToExtReg(sz, Vd, D);
    // VLDR <{S,D}d>, [<Rn>, #+/-<imm32>]
    if (ConditionPassed(cond)) {
        auto base = n == Reg::PC ? ir.Imm32(ir.AlignPC(4)) : ir.GetRegister(n);
        auto address = U ? ir.Add(base, ir.Imm32(imm32)) : ir.Sub(base, ir.Imm32(imm32));
        if (sz) {
            auto lo = ir.ReadMemory32(address);
            auto hi = ir.ReadMemory32(ir.Add(address, ir.Imm32(4)));
            if (ir.current_location.EFlag()) std::swap(lo, hi);
            ir.SetExtendedRegister(d, ir.TransferToFP64(ir.Pack2x32To1x64(lo, hi)));
        } else {
            ir.SetExtendedRegister(d, ir.TransferToFP32(ir.ReadMemory32(address)));
        }
    }
    return true;
}

} // namespace Arm
} // namespace Dynarmic
