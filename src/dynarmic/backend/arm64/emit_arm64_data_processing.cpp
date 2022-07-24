/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <cstddef>

#include <fmt/ostream.h>
#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/a32_jitstate.h"
#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/backend/arm64/emit_arm64.h"
#include "dynarmic/backend/arm64/emit_context.h"
#include "dynarmic/backend/arm64/reg_alloc.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

template<size_t bitsize, typename EmitFn>
static void EmitTwoOp(oaknut::CodeGenerator&, EmitContext& ctx, IR::Inst* inst, EmitFn emit) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Rresult = ctx.reg_alloc.WriteReg<bitsize>(inst);
    auto Roperand = ctx.reg_alloc.ReadReg<bitsize>(args[0]);
    RegAlloc::Realize(Rresult, Roperand);

    emit(Rresult, Roperand);
}

template<>
void EmitIR<IR::Opcode::Pack2x32To1x64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wlo = ctx.reg_alloc.ReadW(args[0]);
    auto Whi = ctx.reg_alloc.ReadW(args[1]);
    auto Xresult = ctx.reg_alloc.WriteX(inst);
    RegAlloc::Realize(Wlo, Whi, Xresult);

    code.MOV(Xresult->toW(), Wlo);  // TODO: Move eliminiation
    code.BFI(Xresult, Whi->toX(), 32, 32);
}

template<>
void EmitIR<IR::Opcode::Pack2x64To1x128>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (args[0].IsInGpr() && args[1].IsInGpr()) {
        auto Xlo = ctx.reg_alloc.ReadX(args[0]);
        auto Xhi = ctx.reg_alloc.ReadX(args[1]);
        auto Qresult = ctx.reg_alloc.WriteQ(inst);
        RegAlloc::Realize(Xlo, Xhi, Qresult);

        code.FMOV(Qresult->toD(), Xlo);
        code.MOV(oaknut::VRegSelector{Qresult->index()}.D()[1], Xhi);
    } else if (args[0].IsInGpr()) {
        auto Xlo = ctx.reg_alloc.ReadX(args[0]);
        auto Dhi = ctx.reg_alloc.ReadD(args[1]);
        auto Qresult = ctx.reg_alloc.WriteQ(inst);
        RegAlloc::Realize(Xlo, Dhi, Qresult);

        code.FMOV(Qresult->toD(), Xlo);
        code.MOV(oaknut::VRegSelector{Qresult->index()}.D()[1], oaknut::VRegSelector{Dhi->index()}.D()[0]);
    } else if (args[1].IsInGpr()) {
        auto Dlo = ctx.reg_alloc.ReadD(args[0]);
        auto Xhi = ctx.reg_alloc.ReadX(args[1]);
        auto Qresult = ctx.reg_alloc.WriteQ(inst);
        RegAlloc::Realize(Dlo, Xhi, Qresult);

        code.FMOV(Qresult->toD(), Dlo);  // TODO: Move eliminiation
        code.MOV(oaknut::VRegSelector{Qresult->index()}.D()[1], Xhi);
    } else {
        auto Dlo = ctx.reg_alloc.ReadD(args[0]);
        auto Dhi = ctx.reg_alloc.ReadD(args[1]);
        auto Qresult = ctx.reg_alloc.WriteQ(inst);
        RegAlloc::Realize(Dlo, Dhi, Qresult);

        code.FMOV(Qresult->toD(), Dlo);  // TODO: Move eliminiation
        code.MOV(oaknut::VRegSelector{Qresult->index()}.D()[1], oaknut::VRegSelector{Dhi->index()}.D()[0]);
    }
}

template<>
void EmitIR<IR::Opcode::LeastSignificantWord>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    auto Xoperand = ctx.reg_alloc.ReadX(args[0]);
    RegAlloc::Realize(Wresult, Xoperand);

    code.MOV(Wresult, Xoperand->toW());  // TODO: Zext elimination
}

template<>
void EmitIR<IR::Opcode::LeastSignificantHalf>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    auto Woperand = ctx.reg_alloc.ReadW(args[0]);
    RegAlloc::Realize(Wresult, Woperand);

    code.UXTH(Wresult, Woperand);  // TODO: Zext elimination
}

template<>
void EmitIR<IR::Opcode::LeastSignificantByte>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    auto Woperand = ctx.reg_alloc.ReadW(args[0]);
    RegAlloc::Realize(Wresult, Woperand);

    code.UXTB(Wresult, Woperand);  // TODO: Zext elimination
}

template<>
void EmitIR<IR::Opcode::MostSignificantWord>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    auto Xoperand = ctx.reg_alloc.ReadX(args[0]);
    RegAlloc::Realize(Wresult, Xoperand);

    code.LSL(Wresult->toX(), Xoperand, 32);
}

template<>
void EmitIR<IR::Opcode::MostSignificantBit>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    auto Woperand = ctx.reg_alloc.ReadW(args[0]);
    RegAlloc::Realize(Wresult, Woperand);

    code.LSR(Wresult, Woperand, 31);
}

template<>
void EmitIR<IR::Opcode::IsZero32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    auto Woperand = ctx.reg_alloc.ReadW(args[0]);
    RegAlloc::Realize(Wresult, Woperand);
    ctx.reg_alloc.SpillFlags();

    code.CMP(Woperand, 0);
    code.CSET(Wresult, EQ);
}

template<>
void EmitIR<IR::Opcode::IsZero64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Wresult = ctx.reg_alloc.WriteW(inst);
    auto Xoperand = ctx.reg_alloc.ReadX(args[0]);
    RegAlloc::Realize(Wresult, Xoperand);
    ctx.reg_alloc.SpillFlags();

    code.CMP(Xoperand, 0);
    code.CSET(Wresult, EQ);
}

template<>
void EmitIR<IR::Opcode::TestBit>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ConditionalSelect32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ConditionalSelect64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ConditionalSelectNZCV>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::LogicalShiftLeft32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];
    auto& carry_arg = args[2];

    if (!carry_inst) {
        if (shift_arg.IsImmediate()) {
            const u8 shift = shift_arg.GetImmediateU8();
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            RegAlloc::Realize(Wresult, Woperand);

            if (shift <= 31) {
                code.LSL(Wresult, Woperand, shift);
            } else {
                code.MOV(Wresult, WZR);
            }
        } else {
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            auto Wshift = ctx.reg_alloc.ReadW(shift_arg);
            RegAlloc::Realize(Wresult, Woperand, Wshift);
            ctx.reg_alloc.SpillFlags();

            code.AND(Wscratch0, Wshift, 0xff);
            code.LSL(Wresult, Woperand, Wscratch0);
            code.CMP(Wscratch0, 32);
            code.CSEL(Wresult, Wresult, WZR, LT);
        }
    } else {
        if (shift_arg.IsImmediate() && shift_arg.GetImmediateU8() == 0) {
            ctx.reg_alloc.DefineAsExisting(inst, operand_arg);
            ctx.reg_alloc.DefineAsExisting(carry_inst, carry_arg);
        } else if (shift_arg.IsImmediate()) {
            // TODO: Use RMIF
            const u8 shift = shift_arg.GetImmediateU8();

            if (shift < 32) {
                auto Wresult = ctx.reg_alloc.WriteW(inst);
                auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
                auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
                RegAlloc::Realize(Wresult, Wcarry_out, Woperand);

                code.UBFX(Wcarry_out, Woperand, 32 - shift, 1);
                code.LSL(Wcarry_out, Wcarry_out, 29);
                code.LSL(Wresult, Woperand, shift);
            } else if (shift > 32) {
                auto Wresult = ctx.reg_alloc.WriteW(inst);
                auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
                RegAlloc::Realize(Wresult, Wcarry_out);

                code.MOV(Wresult, WZR);
                code.MOV(Wcarry_out, WZR);
            } else {
                auto Wresult = ctx.reg_alloc.WriteW(inst);
                auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
                auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
                RegAlloc::Realize(Wresult, Wcarry_out, Woperand);

                code.UBFIZ(Wcarry_out, Woperand, 29, 1);
                code.MOV(Wresult, WZR);
            }
        } else {
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            auto Wshift = ctx.reg_alloc.ReadW(shift_arg);
            auto Wcarry_in = ctx.reg_alloc.ReadW(carry_arg);
            RegAlloc::Realize(Wresult, Wcarry_out, Woperand, Wshift, Wcarry_in);
            ctx.reg_alloc.SpillFlags();

            // TODO: Use RMIF

            oaknut::Label zero, end;

            code.ANDS(Wscratch1, Wshift, 0xff);
            code.B(EQ, zero);

            code.NEG(Wscratch0, Wshift);
            code.LSR(Wcarry_out, Woperand, Wscratch0);
            code.LSL(Wresult, Woperand, Wshift);
            code.UBFIZ(Wcarry_out, Wcarry_out, 29, 1);
            code.CMP(Wscratch1, 32);
            code.CSEL(Wresult, Wresult, WZR, LT);
            code.CSEL(Wcarry_out, Wcarry_out, WZR, LE);
            code.B(end);

            code.l(zero);
            code.MOV(*Wresult, Woperand);
            code.MOV(*Wcarry_out, Wcarry_in);

            code.l(end);
        }
    }
}

template<>
void EmitIR<IR::Opcode::LogicalShiftLeft64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::LogicalShiftRight32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::LogicalShiftRight64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ArithmeticShiftRight32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];
    auto& carry_arg = args[2];

    if (!carry_inst) {
        if (shift_arg.IsImmediate()) {
            const u8 shift = shift_arg.GetImmediateU8();
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            RegAlloc::Realize(Wresult, Woperand);

            code.ASR(Wresult, Woperand, shift <= 31 ? shift : 31);
        } else {
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            auto Wshift = ctx.reg_alloc.ReadW(shift_arg);
            RegAlloc::Realize(Wresult, Woperand, Wshift);
            ctx.reg_alloc.SpillFlags();

            code.AND(Wscratch0, Wshift, 0xff);
            code.MOV(Wscratch1, 31);
            code.CMP(Wscratch0, 31);
            code.CSEL(Wscratch0, Wscratch0, Wscratch1, LS);
            code.ASR(Wresult, Woperand, Wscratch0);
        }
    } else {
        if (shift_arg.IsImmediate() && shift_arg.GetImmediateU8() == 0) {
            ctx.reg_alloc.DefineAsExisting(inst, operand_arg);
            ctx.reg_alloc.DefineAsExisting(carry_inst, carry_arg);
        } else if (shift_arg.IsImmediate()) {
            // TODO: Use RMIF

            const u8 shift = shift_arg.GetImmediateU8();

            if (shift <= 31) {
                auto Wresult = ctx.reg_alloc.WriteW(inst);
                auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
                auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
                RegAlloc::Realize(Wresult, Wcarry_out, Woperand);

                code.UBFX(Wcarry_out, Woperand, shift - 1, 1);
                code.LSL(Wcarry_out, Wcarry_out, 29);
                code.ASR(Wresult, Woperand, shift);
            } else {
                auto Wresult = ctx.reg_alloc.WriteW(inst);
                auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
                auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
                RegAlloc::Realize(Wresult, Wcarry_out, Woperand);

                code.ASR(Wresult, Woperand, 31);
                code.AND(Wcarry_out, Wresult, 1 << 29);
            }
        } else {
            auto Wresult = ctx.reg_alloc.WriteW(inst);
            auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
            auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
            auto Wshift = ctx.reg_alloc.ReadW(shift_arg);
            auto Wcarry_in = ctx.reg_alloc.ReadW(carry_arg);
            RegAlloc::Realize(Wresult, Wcarry_out, Woperand, Wshift, Wcarry_in);
            ctx.reg_alloc.SpillFlags();

            // TODO: Use RMIF

            oaknut::Label zero, end;

            code.ANDS(Wscratch0, Wshift, 0xff);
            code.B(EQ, zero);

            code.MOV(Wscratch1, 31);
            code.CMP(Wscratch0, 31);
            code.CSEL(Wscratch0, Wscratch0, Wscratch1, LO);
            code.SUB(Wscratch1, Wscratch0, 1);
            code.ASR(Wresult, Woperand, Wscratch0);
            code.LSR(Wcarry_out, Woperand, Wscratch1);
            code.UBFIZ(Wcarry_out, Wcarry_out, 29, 1);
            code.B(end);

            code.l(zero);
            code.MOV(*Wresult, Woperand);
            code.MOV(*Wcarry_out, Wcarry_in);

            code.l(end);
        }
    }
}

template<>
void EmitIR<IR::Opcode::ArithmeticShiftRight64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::RotateRight32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];
    auto& carry_arg = args[2];

    if (shift_arg.IsImmediate() && shift_arg.GetImmediateU8() == 0) {
        ctx.reg_alloc.DefineAsExisting(inst, operand_arg);

        if (carry_inst) {
            ctx.reg_alloc.DefineAsExisting(carry_inst, carry_arg);
        }
    } else if (shift_arg.IsImmediate()) {
        const u8 shift = shift_arg.GetImmediateU8() % 32;
        auto Wresult = ctx.reg_alloc.WriteW(inst);
        auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
        RegAlloc::Realize(Wresult, Woperand);

        code.ROR(Wresult, Woperand, shift);

        if (carry_inst) {
            auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
            RegAlloc::Realize(Wcarry_out);

            code.ROR(Wcarry_out, Woperand, ((shift + 31) - 29) % 32);
            code.AND(Wcarry_out, Wcarry_out, 1 << 29);
        }
    } else {
        auto Wresult = ctx.reg_alloc.WriteW(inst);
        auto Woperand = ctx.reg_alloc.ReadW(operand_arg);
        auto Wshift = ctx.reg_alloc.ReadW(shift_arg);
        RegAlloc::Realize(Wresult, Woperand, Wshift);

        code.ROR(Wresult, Woperand, Wshift);

        if (carry_inst) {
            auto Wcarry_in = ctx.reg_alloc.ReadW(carry_arg);
            auto Wcarry_out = ctx.reg_alloc.WriteW(carry_inst);
            RegAlloc::Realize(Wcarry_out, Wcarry_in);
            ctx.reg_alloc.SpillFlags();

            code.TST(Wshift, 0xff);
            code.LSR(Wcarry_out, Wresult, 31 - 29);
            code.AND(Wcarry_out, Wcarry_out, 1 << 29);
            code.CSEL(Wcarry_out, Wcarry_in, Wcarry_out, EQ);
        }
    }
}

template<>
void EmitIR<IR::Opcode::RotateRight64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::RotateRightExtended>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::LogicalShiftLeftMasked32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::LogicalShiftLeftMasked64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::LogicalShiftRightMasked32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::LogicalShiftRightMasked64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ArithmeticShiftRightMasked32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ArithmeticShiftRightMasked64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::RotateRightMasked32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::RotateRightMasked64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<size_t bitsize, typename EmitFn>
static void MaybeAddSubImm(oaknut::CodeGenerator& code, u64 imm, EmitFn emit_fn) {
    static_assert(bitsize == 32 || bitsize == 64);
    if constexpr (bitsize == 32) {
        imm = static_cast<u32>(imm);
    }
    if (oaknut::AddSubImm::is_valid(imm)) {
        emit_fn(imm);
    } else {
        code.MOV(Rscratch0<bitsize>(), imm);
        emit_fn(Rscratch0<bitsize>());
    }
}

template<size_t bitsize, bool sub>
static void EmitAddSub(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const auto nzcv_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetNZCVFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    auto Rresult = ctx.reg_alloc.WriteReg<bitsize>(inst);
    auto Ra = ctx.reg_alloc.ReadReg<bitsize>(args[0]);

    if (nzcv_inst) {
        if (args[1].IsImmediate()) {
            const u64 imm = args[1].GetImmediateU64();

            if (args[2].IsImmediate()) {
                auto flags = ctx.reg_alloc.WriteFlags(nzcv_inst);
                RegAlloc::Realize(Rresult, Ra, flags);

                if (args[2].GetImmediateU1()) {
                    MaybeAddSubImm<bitsize>(code, sub ? imm : ~imm, [&](const auto b) { code.SUBS(Rresult, *Ra, b); });
                } else {
                    MaybeAddSubImm<bitsize>(code, sub ? ~imm : imm, [&](const auto b) { code.ADDS(Rresult, *Ra, b); });
                }
            } else {
                RegAlloc::Realize(Rresult, Ra);
                ctx.reg_alloc.ReadWriteFlags(args[2], nzcv_inst);

                code.MOV(Rscratch0<bitsize>(), imm);
                sub ? code.SBCS(Rresult, Ra, Rscratch0<bitsize>()) : code.ADCS(Rresult, Ra, Rscratch0<bitsize>());
            }
        } else {
            auto Rb = ctx.reg_alloc.ReadReg<bitsize>(args[1]);

            if (args[2].IsImmediate()) {
                auto flags = ctx.reg_alloc.WriteFlags(nzcv_inst);
                RegAlloc::Realize(Rresult, Ra, Rb, flags);

                if (args[2].GetImmediateU1()) {
                    if (sub) {
                        code.SUBS(Rresult, *Ra, Rb);
                    } else {
                        code.MVN(Rscratch0<bitsize>(), Rb);
                        code.SUBS(Rresult, *Ra, Rscratch0<bitsize>());
                    }
                } else {
                    if (sub) {
                        code.MVN(Rscratch0<bitsize>(), Rb);
                        code.ADDS(Rresult, *Ra, Rscratch0<bitsize>());
                    } else {
                        code.ADDS(Rresult, *Ra, Rb);
                    }
                }
            } else {
                RegAlloc::Realize(Rresult, Ra, Rb);
                ctx.reg_alloc.ReadWriteFlags(args[2], nzcv_inst);

                sub ? code.SBCS(Rresult, Ra, Rb) : code.ADCS(Rresult, Ra, Rb);
            }
        }
    } else {
        if (args[1].IsImmediate()) {
            const u64 imm = args[1].GetImmediateU64();

            RegAlloc::Realize(Rresult, Ra);

            if (args[2].IsImmediate()) {
                if (args[2].GetImmediateU1()) {
                    MaybeAddSubImm<bitsize>(code, sub ? imm : ~imm, [&](const auto b) { code.SUB(Rresult, *Ra, b); });
                } else {
                    MaybeAddSubImm<bitsize>(code, sub ? ~imm : imm, [&](const auto b) { code.ADD(Rresult, *Ra, b); });
                }
            } else {
                code.MOV(Rscratch0<bitsize>(), imm);
                sub ? code.SBC(Rresult, Ra, Rscratch0<bitsize>()) : code.ADC(Rresult, Ra, Rscratch0<bitsize>());
            }
        } else {
            auto Rb = ctx.reg_alloc.ReadReg<bitsize>(args[1]);

            RegAlloc::Realize(Rresult, Ra, Rb);

            if (args[2].IsImmediate()) {
                if (args[2].GetImmediateU1()) {
                    if (sub) {
                        code.SUB(Rresult, *Ra, Rb);
                    } else {
                        code.MVN(Rscratch0<bitsize>(), Rb);
                        code.SUB(Rresult, *Ra, Rscratch0<bitsize>());
                    }
                } else {
                    if (sub) {
                        code.MVN(Rscratch0<bitsize>(), Rb);
                        code.ADD(Rresult, *Ra, Rscratch0<bitsize>());
                    } else {
                        code.ADD(Rresult, *Ra, Rb);
                    }
                }
            } else {
                sub ? code.SBC(Rresult, Ra, Rb) : code.ADC(Rresult, Ra, Rb);
            }
        }
    }
}

template<>
void EmitIR<IR::Opcode::Add32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitAddSub<32, false>(code, ctx, inst);
}

template<>
void EmitIR<IR::Opcode::Add64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitAddSub<64, false>(code, ctx, inst);
}

template<>
void EmitIR<IR::Opcode::Sub32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitAddSub<32, true>(code, ctx, inst);
}

template<>
void EmitIR<IR::Opcode::Sub64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitAddSub<64, true>(code, ctx, inst);
}

template<>
void EmitIR<IR::Opcode::Mul32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::Mul64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::SignedMultiplyHigh64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::UnsignedMultiplyHigh64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::UnsignedDiv32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::UnsignedDiv64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::SignedDiv32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::SignedDiv64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<size_t bitsize, typename EmitFn>
static void MaybeBitImm(oaknut::CodeGenerator& code, u64 imm, EmitFn emit_fn) {
    static_assert(bitsize == 32 || bitsize == 64);
    if constexpr (bitsize == 32) {
        imm = static_cast<u32>(imm);
    }
    if (oaknut::detail::encode_bit_imm(imm)) {
        emit_fn(imm);
    } else {
        code.MOV(Rscratch0<bitsize>(), imm);
        emit_fn(Rscratch0<bitsize>());
    }
}

template<size_t bitsize, typename EmitFn1, typename EmitFn2 = std::nullptr_t>
static void EmitBitOp(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, EmitFn1 emit_without_flags, EmitFn2 emit_with_flags = nullptr) {
    const auto nz_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetNZFromOp);
    const auto nzcv_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetNZCVFromOp);
    ASSERT(!(nz_inst && nzcv_inst));
    const auto flag_inst = nz_inst ? nz_inst : nzcv_inst;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto Rresult = ctx.reg_alloc.WriteReg<bitsize>(inst);
    auto Ra = ctx.reg_alloc.ReadReg<bitsize>(args[0]);

    if constexpr (!std::is_same_v<EmitFn2, std::nullptr_t>) {
        if (flag_inst) {
            auto Wflags = ctx.reg_alloc.WriteFlags(flag_inst);

            if (args[1].IsImmediate()) {
                RegAlloc::Realize(Rresult, Ra, Wflags);

                MaybeBitImm<bitsize>(code, args[1].GetImmediateU64(), [&](const auto& b) { emit_with_flags(Rresult, Ra, b); });
            } else {
                auto Rb = ctx.reg_alloc.ReadReg<bitsize>(args[1]);
                RegAlloc::Realize(Rresult, Ra, Rb, Wflags);

                emit_with_flags(Rresult, Ra, Rb);
            }

            return;
        }
    }

    if (args[1].IsImmediate()) {
        RegAlloc::Realize(Rresult, Ra);

        MaybeBitImm<bitsize>(code, args[1].GetImmediateU64(), [&](const auto& b) { emit_without_flags(Rresult, Ra, b); });
    } else {
        auto Rb = ctx.reg_alloc.ReadReg<bitsize>(args[1]);
        RegAlloc::Realize(Rresult, Ra, Rb);

        emit_without_flags(Rresult, Rb, Rb);
    }
}

template<size_t bitsize>
static void EmitAndNot(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    const auto nz_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetNZFromOp);
    const auto nzcv_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetNZCVFromOp);
    ASSERT(!(nz_inst && nzcv_inst));
    const auto flag_inst = nz_inst ? nz_inst : nzcv_inst;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto Rresult = ctx.reg_alloc.WriteReg<bitsize>(inst);
    auto Ra = ctx.reg_alloc.ReadReg<bitsize>(args[0]);

    if (flag_inst) {
        auto Wflags = ctx.reg_alloc.WriteFlags(flag_inst);

        if (args[1].IsImmediate()) {
            RegAlloc::Realize(Rresult, Ra, Wflags);

            const u64 not_imm = bitsize == 32 ? static_cast<u32>(~args[1].GetImmediateU64()) : ~args[1].GetImmediateU64();

            if (oaknut::detail::encode_bit_imm(not_imm)) {
                code.ANDS(Rresult, Ra, not_imm);
            } else {
                code.MOV(Rscratch0<bitsize>(), args[1].GetImmediateU64());
                code.BICS(Rresult, Ra, Rscratch0<bitsize>());
            }
        } else {
            auto Rb = ctx.reg_alloc.ReadReg<bitsize>(args[1]);
            RegAlloc::Realize(Rresult, Ra, Rb, Wflags);

            code.BICS(Rresult, Ra, Rb);
        }

        return;
    }

    if (args[1].IsImmediate()) {
        RegAlloc::Realize(Rresult, Ra);

        const u64 not_imm = bitsize == 32 ? static_cast<u32>(~args[1].GetImmediateU64()) : ~args[1].GetImmediateU64();

        if (oaknut::detail::encode_bit_imm(not_imm)) {
            code.AND(Rresult, Ra, not_imm);
        } else {
            code.MOV(Rscratch0<bitsize>(), args[1].GetImmediateU64());
            code.BIC(Rresult, Ra, Rscratch0<bitsize>());
        }
    } else {
        auto Rb = ctx.reg_alloc.ReadReg<bitsize>(args[1]);
        RegAlloc::Realize(Rresult, Ra, Rb);

        code.BIC(Rresult, Ra, Rb);
    }
}

template<>
void EmitIR<IR::Opcode::And32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitBitOp<32>(
        code, ctx, inst,
        [&](auto& result, auto& a, auto& b) { code.AND(result, a, b); },
        [&](auto& result, auto& a, auto& b) { code.ANDS(result, a, b); });
}

template<>
void EmitIR<IR::Opcode::And64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitBitOp<64>(
        code, ctx, inst,
        [&](auto& result, auto& a, auto& b) { code.AND(result, a, b); },
        [&](auto& result, auto& a, auto& b) { code.ANDS(result, a, b); });
}

template<>
void EmitIR<IR::Opcode::AndNot32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitAndNot<32>(code, ctx, inst);
}

template<>
void EmitIR<IR::Opcode::AndNot64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitAndNot<32>(code, ctx, inst);
}

template<>
void EmitIR<IR::Opcode::Eor32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitBitOp<32>(
        code, ctx, inst,
        [&](auto& result, auto& a, auto& b) { code.EOR(result, a, b); });
}

template<>
void EmitIR<IR::Opcode::Eor64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitBitOp<64>(
        code, ctx, inst,
        [&](auto& result, auto& a, auto& b) { code.EOR(result, a, b); });
}

template<>
void EmitIR<IR::Opcode::Or32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitBitOp<32>(
        code, ctx, inst,
        [&](auto& result, auto& a, auto& b) { code.ORR(result, a, b); });
}

template<>
void EmitIR<IR::Opcode::Or64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitBitOp<64>(
        code, ctx, inst,
        [&](auto& result, auto& a, auto& b) { code.ORR(result, a, b); });
}

template<>
void EmitIR<IR::Opcode::Not32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<32>(
        code, ctx, inst,
        [&](auto& Wresult, auto& Woperand) { code.MVN(Wresult, Woperand); });
}

template<>
void EmitIR<IR::Opcode::Not64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<64>(
        code, ctx, inst,
        [&](auto& Xresult, auto& Xoperand) { code.MVN(Xresult, Xoperand); });
}

template<>
void EmitIR<IR::Opcode::SignExtendByteToWord>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<32>(
        code, ctx, inst,
        [&](auto& Wresult, auto& Woperand) { code.SXTB(Wresult, Woperand); });
}

template<>
void EmitIR<IR::Opcode::SignExtendHalfToWord>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<32>(
        code, ctx, inst,
        [&](auto& Wresult, auto& Woperand) { code.SXTH(Wresult, Woperand); });
}

template<>
void EmitIR<IR::Opcode::SignExtendByteToLong>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<64>(
        code, ctx, inst,
        [&](auto& Xresult, auto& Xoperand) { code.SXTB(Xresult, Xoperand->toW()); });
}

template<>
void EmitIR<IR::Opcode::SignExtendHalfToLong>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<64>(
        code, ctx, inst,
        [&](auto& Xresult, auto& Xoperand) { code.SXTH(Xresult, Xoperand->toW()); });
}

template<>
void EmitIR<IR::Opcode::SignExtendWordToLong>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<64>(
        code, ctx, inst,
        [&](auto& Xresult, auto& Xoperand) { code.SXTW(Xresult, Xoperand->toW()); });
}

template<>
void EmitIR<IR::Opcode::ZeroExtendByteToWord>(oaknut::CodeGenerator&, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.DefineAsExisting(inst, args[0]);
}

template<>
void EmitIR<IR::Opcode::ZeroExtendHalfToWord>(oaknut::CodeGenerator&, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.DefineAsExisting(inst, args[0]);
}

template<>
void EmitIR<IR::Opcode::ZeroExtendByteToLong>(oaknut::CodeGenerator&, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.DefineAsExisting(inst, args[0]);
}

template<>
void EmitIR<IR::Opcode::ZeroExtendHalfToLong>(oaknut::CodeGenerator&, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.DefineAsExisting(inst, args[0]);
}

template<>
void EmitIR<IR::Opcode::ZeroExtendWordToLong>(oaknut::CodeGenerator&, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.DefineAsExisting(inst, args[0]);
}

template<>
void EmitIR<IR::Opcode::ZeroExtendLongToQuad>(oaknut::CodeGenerator&, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.DefineAsExisting(inst, args[0]);
}

template<>
void EmitIR<IR::Opcode::ByteReverseWord>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<32>(
        code, ctx, inst,
        [&](auto& Wresult, auto& Woperand) { code.REV(Wresult, Woperand); });
}

template<>
void EmitIR<IR::Opcode::ByteReverseHalf>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<32>(
        code, ctx, inst,
        [&](auto& Wresult, auto& Woperand) { code.REV16(Wresult, Woperand); });
}

template<>
void EmitIR<IR::Opcode::ByteReverseDual>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOp<64>(
        code, ctx, inst,
        [&](auto& Xresult, auto& Xoperand) { code.REV(Xresult, Xoperand); });
}

template<>
void EmitIR<IR::Opcode::CountLeadingZeros32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::CountLeadingZeros64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ExtractRegister32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ExtractRegister64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ReplicateBit32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::ReplicateBit64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::MaxSigned32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::MaxSigned64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::MaxUnsigned32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::MaxUnsigned64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::MinSigned32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::MinSigned64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::MinUnsigned32>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

template<>
void EmitIR<IR::Opcode::MinUnsigned64>(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst) {
    (void)code;
    (void)ctx;
    (void)inst;
    ASSERT_FALSE("Unimplemented");
}

}  // namespace Dynarmic::Backend::Arm64
