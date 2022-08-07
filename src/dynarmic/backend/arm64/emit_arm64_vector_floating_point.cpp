/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/a32_jitstate.h"
#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/backend/arm64/emit_context.h"
#include "dynarmic/backend/arm64/fpsr_manager.h"
#include "dynarmic/backend/arm64/reg_alloc.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

template<typename EmitFn>
static void MaybeStandardFPSCRValue(oaknut::CodeGenerator& code, EmitContext& ctx, bool fpcr_controlled, EmitFn emit) {
    if (ctx.FPCR(fpcr_controlled) != ctx.FPCR()) {
        code.MOV(Wscratch0, ctx.FPCR(fpcr_controlled).Value());
        code.MSR(oaknut::SystemReg::FPCR, Xscratch0);
        emit();
        code.MOV(Wscratch0, ctx.FPCR().Value());
        code.MSR(oaknut::SystemReg::FPCR, Xscratch0);
    } else {
        emit();
    }
}

template<typename EmitFn>
static void EmitTwoOp(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, EmitFn emit) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto Qresult = ctx.reg_alloc.WriteQ(inst);
    auto Qa = ctx.reg_alloc.ReadQ(args[0]);
    const bool fpcr_controlled = args[1].IsVoid() || args[1].GetImmediateU1();
    RegAlloc::Realize(Qresult, Qa);
    ctx.fpsr.Load();

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] { emit(Qresult, Qa); });
}

template<size_t size, typename EmitFn>
static void EmitTwoOpArranged(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, EmitFn emit) {
    EmitTwoOp(code, ctx, inst, [&](auto& Qresult, auto& Qa) {
        if constexpr (size == 16) {
            emit(Qresult->H8(), Qa->H8());
        } else if constexpr (size == 32) {
            emit(Qresult->S4(), Qa->S4());
        } else if constexpr (size == 64) {
            emit(Qresult->D2(), Qa->D2());
        } else {
            static_assert(size == 16 || size == 32 || size == 64);
        }
    });
}

template<typename EmitFn>
static void EmitThreeOp(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, EmitFn emit) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto Qresult = ctx.reg_alloc.WriteQ(inst);
    auto Qa = ctx.reg_alloc.ReadQ(args[0]);
    auto Qb = ctx.reg_alloc.ReadQ(args[1]);
    const bool fpcr_controlled = args[2].GetImmediateU1();
    RegAlloc::Realize(Qresult, Qa, Qb);
    ctx.fpsr.Load();

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] { emit(Qresult, Qa, Qb); });
}

template<size_t size, typename EmitFn>
static void EmitThreeOpArranged(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, EmitFn emit) {
    EmitThreeOp(code, ctx, inst, [&](auto& Qresult, auto& Qa, auto& Qb) {
        if constexpr (size == 16) {
            emit(Qresult->H8(), Qa->H8(), Qb->H8());
        } else if constexpr (size == 32) {
            emit(Qresult->S4(), Qa->S4(), Qb->S4());
        } else if constexpr (size == 64) {
            emit(Qresult->D2(), Qa->D2(), Qb->D2());
        } else {
            static_assert(size == 16 || size == 32 || size == 64);
        }
    });
}

template<size_t size, typename EmitFn>
static void EmitFMA(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, EmitFn emit) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto Qresult = ctx.reg_alloc.ReadWriteQ(args[0], inst);
    auto Qm = ctx.reg_alloc.ReadQ(args[1]);
    auto Qn = ctx.reg_alloc.ReadQ(args[2]);
    const bool fpcr_controlled = args[3].GetImmediateU1();
    RegAlloc::Realize(Qresult, Qm, Qn);
    ctx.fpsr.Load();

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        if constexpr (size == 16) {
            emit(Qresult->H8(), Qm->H8(), Qn->H8());
        } else if constexpr (size == 32) {
            emit(Qresult->S4(), Qm->S4(), Qn->S4());
        } else if constexpr (size == 64) {
            emit(Qresult->D2(), Qm->D2(), Qn->D2());
        } else {
            static_assert(size == 16 || size == 32 || size == 64);
        }
    });
}

template<size_t size, typename EmitFn>
static void EmitFromFixed(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, EmitFn emit) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto Qto = ctx.reg_alloc.WriteQ(inst);
    auto Qfrom = ctx.reg_alloc.ReadQ(args[0]);
    const u8 fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    const bool fpcr_controlled = args[3].GetImmediateU1();
    ASSERT(rounding_mode == ctx.FPCR(fpcr_controlled).RMode());
    RegAlloc::Realize(Qto, Qfrom);

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        if constexpr (size == 32) {
            emit(Qto->S4(), Qfrom->S4(), fbits);
        } else if constexpr (size == 64) {
            emit(Qto->D2(), Qfrom->D2(), fbits);
        } else {
            static_assert(size == 32 || size == 64);
        }
    });
}

template<size_t fsize, bool is_signed>
void EmitToFixed(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto Qto = ctx.reg_alloc.WriteQ(inst);
    auto Qfrom = ctx.reg_alloc.ReadQ(args[0]);
    const size_t fbits = args[1].GetImmediateU8();
    const auto rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    const bool fpcr_controlled = inst->GetArg(3).GetU1();
    RegAlloc::Realize(Qto, Qfrom);
    ctx.fpsr.Load();

    auto Vto = [&] {
        if constexpr (fsize == 32) {
            return Qto->S4();
        } else if constexpr (fsize == 64) {
            return Qto->D2();
        } else {
            static_assert(fsize == 32 || fsize == 64);
        }
    }();
    auto Vfrom = [&] {
        if constexpr (fsize == 32) {
            return Qfrom->S4();
        } else if constexpr (fsize == 64) {
            return Qfrom->D2();
        } else {
            static_assert(fsize == 32 || fsize == 64);
        }
    }();

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        if (rounding_mode == FP::RoundingMode::TowardsZero) {
            if constexpr (is_signed) {
                if (fbits) {
                    code.FCVTZS(Vto, Vfrom, fbits);
                } else {
                    code.FCVTZS(Vto, Vfrom);
                }
            } else {
                if (fbits) {
                    code.FCVTZU(Vto, Vfrom, fbits);
                } else {
                    code.FCVTZU(Vto, Vfrom);
                }
            }
        } else {
            ASSERT(fbits == 0);
            if constexpr (is_signed) {
                switch (rounding_mode) {
                case FP::RoundingMode::ToNearest_TieEven:
                    code.FCVTNS(Vto, Vfrom);
                    break;
                case FP::RoundingMode::TowardsPlusInfinity:
                    code.FCVTPS(Vto, Vfrom);
                    break;
                case FP::RoundingMode::TowardsMinusInfinity:
                    code.FCVTMS(Vto, Vfrom);
                    break;
                case FP::RoundingMode::TowardsZero:
                    code.FCVTZS(Vto, Vfrom);
                    break;
                case FP::RoundingMode::ToNearest_TieAwayFromZero:
                    code.FCVTAS(Vto, Vfrom);
                    break;
                case FP::RoundingMode::ToOdd:
                    ASSERT_FALSE("Unimplemented");
                    break;
                default:
                    ASSERT_FALSE("Invalid RoundingMode");
                    break;
                }
            } else {
                switch (rounding_mode) {
                case FP::RoundingMode::ToNearest_TieEven:
                    code.FCVTNU(Vto, Vfrom);
                    break;
                case FP::RoundingMode::TowardsPlusInfinity:
                    code.FCVTPU(Vto, Vfrom);
                    break;
                case FP::RoundingMode::TowardsMinusInfinity:
                    code.FCVTMU(Vto, Vfrom);
                    break;
                case FP::RoundingMode::TowardsZero:
                    code.FCVTZU(Vto, Vfrom);
                    break;
                case FP::RoundingMode::ToNearest_TieAwayFromZero:
                    code.FCVTAU(Vto, Vfrom);
                    break;
                case FP::RoundingMode::ToOdd:
                    ASSERT_FALSE("Unimplemented");
                    break;
                default:
                    ASSERT_FALSE("Invalid RoundingMode");
                    break;
                }
            }
        }
    });
}

template<>
void EmitIR<IR::Opcode::FPVectorAbs16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorAbs32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va) { code.FABS(Vresult, Va); });
}

template<>
void EmitIR<IR::Opcode::FPVectorAbs64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va) { code.FABS(Vresult, Va); });
}

template<>
void EmitIR<IR::Opcode::FPVectorAdd32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FADD(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorAdd64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FADD(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorDiv32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FDIV(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorDiv64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FDIV(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorEqual16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorEqual32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FCMEQ(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorEqual64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FCMEQ(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorFromHalf32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorFromSignedFixed32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitFromFixed<32>(code, ctx, inst, [&](auto Vto, auto Vfrom, u8 fbits) { fbits ? code.SCVTF(Vto, Vfrom, fbits) : code.SCVTF(Vto, Vfrom); });
}

template<>
void EmitIR<IR::Opcode::FPVectorFromSignedFixed64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitFromFixed<64>(code, ctx, inst, [&](auto Vto, auto Vfrom, u8 fbits) { fbits ? code.SCVTF(Vto, Vfrom, fbits) : code.SCVTF(Vto, Vfrom); });
}

template<>
void EmitIR<IR::Opcode::FPVectorFromUnsignedFixed32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitFromFixed<32>(code, ctx, inst, [&](auto Vto, auto Vfrom, u8 fbits) { fbits ? code.UCVTF(Vto, Vfrom, fbits) : code.UCVTF(Vto, Vfrom); });
}

template<>
void EmitIR<IR::Opcode::FPVectorFromUnsignedFixed64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitFromFixed<64>(code, ctx, inst, [&](auto Vto, auto Vfrom, u8 fbits) { fbits ? code.UCVTF(Vto, Vfrom, fbits) : code.UCVTF(Vto, Vfrom); });
}

template<>
void EmitIR<IR::Opcode::FPVectorGreater32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FCMGT(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorGreater64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FCMGT(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorGreaterEqual32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FCMGE(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorGreaterEqual64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FCMGE(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMax32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FMAX(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMax64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FMAX(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMin32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FMIN(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMin64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FMIN(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMul32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FMUL(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMul64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FMUL(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMulAdd16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorMulAdd32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitFMA<32>(code, ctx, inst, [&](auto Va, auto Vn, auto Vm) { code.FMLA(Va, Vn, Vm); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMulAdd64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitFMA<64>(code, ctx, inst, [&](auto Va, auto Vn, auto Vm) { code.FMLA(Va, Vn, Vm); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMulX32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FMULX(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorMulX64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FMULX(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorNeg16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorNeg32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va) { code.FNEG(Vresult, Va); });
}

template<>
void EmitIR<IR::Opcode::FPVectorNeg64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va) { code.FNEG(Vresult, Va); });
}

template<>
void EmitIR<IR::Opcode::FPVectorPairedAdd32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FADDP(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorPairedAdd64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FADDP(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorPairedAddLower32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOp(code, ctx, inst, [&](auto& Qresult, auto& Qa, auto& Qb) {
        code.ZIP1(V0.D2(), Qa->D2(), Qb->D2());
        code.MOVI(D1, oaknut::RepImm{0});
        code.FADDP(Qresult->S4(), V0.S4(), V1.S4());
    });
}

template<>
void EmitIR<IR::Opcode::FPVectorPairedAddLower64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOp(code, ctx, inst, [&](auto& Qresult, auto& Qa, auto& Qb) {
        code.ZIP1(V0.D2(), Qa->D2(), Qb->D2());
        code.FADDP(Qresult->toD(), V0.D2());
    });
}

template<>
void EmitIR<IR::Opcode::FPVectorRecipEstimate16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorRecipEstimate32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Voperand) { code.FRECPE(Vresult, Voperand); });
}

template<>
void EmitIR<IR::Opcode::FPVectorRecipEstimate64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Voperand) { code.FRECPE(Vresult, Voperand); });
}

template<>
void EmitIR<IR::Opcode::FPVectorRecipStepFused16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorRecipStepFused32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FRECPS(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorRecipStepFused64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FRECPS(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorRoundInt16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorRoundInt32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const auto rounding_mode = static_cast<FP::RoundingMode>(inst->GetArg(1).GetU8());
    const bool exact = inst->GetArg(2).GetU1();
    const bool fpcr_controlled = inst->GetArg(3).GetU1();

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto Qresult = ctx.reg_alloc.WriteQ(inst);
    auto Qoperand = ctx.reg_alloc.ReadQ(args[0]);
    RegAlloc::Realize(Qresult, Qoperand);
    ctx.fpsr.Load();

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        if (exact) {
            ASSERT(ctx.FPCR().RMode() == rounding_mode);
            code.FRINTX(Qresult->S4(), Qoperand->S4());
        } else {
            switch (rounding_mode) {
            case FP::RoundingMode::ToNearest_TieEven:
                code.FRINTN(Qresult->S4(), Qoperand->S4());
                break;
            case FP::RoundingMode::TowardsPlusInfinity:
                code.FRINTP(Qresult->S4(), Qoperand->S4());
                break;
            case FP::RoundingMode::TowardsMinusInfinity:
                code.FRINTM(Qresult->S4(), Qoperand->S4());
                break;
            case FP::RoundingMode::TowardsZero:
                code.FRINTZ(Qresult->S4(), Qoperand->S4());
                break;
            case FP::RoundingMode::ToNearest_TieAwayFromZero:
                code.FRINTA(Qresult->S4(), Qoperand->S4());
                break;
            default:
                ASSERT_FALSE("Invalid RoundingMode");
            }
        }
    });
}

template<>
void EmitIR<IR::Opcode::FPVectorRoundInt64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const auto rounding_mode = static_cast<FP::RoundingMode>(inst->GetArg(1).GetU8());
    const bool exact = inst->GetArg(2).GetU1();
    const bool fpcr_controlled = inst->GetArg(3).GetU1();

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto Qresult = ctx.reg_alloc.WriteQ(inst);
    auto Qoperand = ctx.reg_alloc.ReadQ(args[0]);
    RegAlloc::Realize(Qresult, Qoperand);
    ctx.fpsr.Load();

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        if (exact) {
            ASSERT(ctx.FPCR().RMode() == rounding_mode);
            code.FRINTX(Qresult->D2(), Qoperand->D2());
        } else {
            switch (rounding_mode) {
            case FP::RoundingMode::ToNearest_TieEven:
                code.FRINTN(Qresult->D2(), Qoperand->D2());
                break;
            case FP::RoundingMode::TowardsPlusInfinity:
                code.FRINTP(Qresult->D2(), Qoperand->D2());
                break;
            case FP::RoundingMode::TowardsMinusInfinity:
                code.FRINTM(Qresult->D2(), Qoperand->D2());
                break;
            case FP::RoundingMode::TowardsZero:
                code.FRINTZ(Qresult->D2(), Qoperand->D2());
                break;
            case FP::RoundingMode::ToNearest_TieAwayFromZero:
                code.FRINTA(Qresult->D2(), Qoperand->D2());
                break;
            default:
                ASSERT_FALSE("Invalid RoundingMode");
            }
        }
    });
}

template<>
void EmitIR<IR::Opcode::FPVectorRSqrtEstimate16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorRSqrtEstimate32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Voperand) { code.FRSQRTE(Vresult, Voperand); });
}

template<>
void EmitIR<IR::Opcode::FPVectorRSqrtEstimate64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Voperand) { code.FRSQRTE(Vresult, Voperand); });
}

template<>
void EmitIR<IR::Opcode::FPVectorRSqrtStepFused16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorRSqrtStepFused32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FRSQRTS(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorRSqrtStepFused64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FRSQRTS(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorSqrt32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va) { code.FSQRT(Vresult, Va); });
}

template<>
void EmitIR<IR::Opcode::FPVectorSqrt64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va) { code.FSQRT(Vresult, Va); });
}

template<>
void EmitIR<IR::Opcode::FPVectorSub32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<32>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FSUB(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorSub64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpArranged<64>(code, ctx, inst, [&](auto Vresult, auto Va, auto Vb) { code.FSUB(Vresult, Va, Vb); });
}

template<>
void EmitIR<IR::Opcode::FPVectorToHalf32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorToSignedFixed16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorToSignedFixed32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitToFixed<32, true>(code, ctx, inst);
}

template<>
void EmitIR<IR::Opcode::FPVectorToSignedFixed64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitToFixed<64, true>(code, ctx, inst);
}

template<>
void EmitIR<IR::Opcode::FPVectorToUnsignedFixed16>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::FPVectorToUnsignedFixed32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitToFixed<32, false>(code, ctx, inst);
}

template<>
void EmitIR<IR::Opcode::FPVectorToUnsignedFixed64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitToFixed<64, false>(code, ctx, inst);
}

}  // namespace Dynarmic::Backend::Arm64
