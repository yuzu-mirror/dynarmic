/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "backend/x64/block_of_code.h"
#include "backend/x64/emit_x64.h"
#include "common/common_types.h"
#include "ir/microinstruction.h"
#include "ir/opcodes.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;

namespace {

void EmitVectorSaturatedNative(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, void (Xbyak::CodeGenerator::*saturated_fn)(const Xbyak::Mmx& mmx, const Xbyak::Operand&), void (Xbyak::CodeGenerator::*unsaturated_fn)(const Xbyak::Mmx& mmx, const Xbyak::Operand&), void (Xbyak::CodeGenerator::*sub_fn)(const Xbyak::Mmx& mmx, const Xbyak::Operand&)) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm addend = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Reg8 overflow = ctx.reg_alloc.ScratchGpr().cvt8();

    code.movaps(xmm0, result);

    (code.*saturated_fn)(result, addend);

    (code.*unsaturated_fn)(xmm0, addend);
    (code.*sub_fn)(xmm0, result);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.ptest(xmm0, xmm0);
    } else {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
        code.pxor(tmp, tmp);
        code.pcmpeqw(xmm0, tmp);
        code.pmovmskb(overflow.cvt32(), xmm0);
        code.xor_(overflow.cvt32(), 0xFFFF);
        code.test(overflow.cvt32(), overflow.cvt32());
    }
    code.setnz(overflow);
    code.or_(code.byte[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], overflow);

    ctx.reg_alloc.DefineValue(inst, result);
}

enum class Op {
    Add,
    Sub,
};

template<Op op, size_t esize>
void EmitVectorSignedSaturated(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    static_assert(esize == 32 || esize == 64);
    constexpr u64 msb_mask = esize == 32 ? 0x8000000080000000 : 0x8000000000000000;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm arg = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Reg8 overflow = ctx.reg_alloc.ScratchGpr().cvt8();

    // TODO AVX-512: vpternlog, vpsraq
    // TODO AVX2 implementation

    code.movaps(xmm0, result);
    code.movaps(tmp, result);

    if constexpr (op == Op::Add) {
        if constexpr (esize == 32) {
            code.paddd(result, arg);
        } else {
            code.paddq(result, arg);
        }
    } else {
        if constexpr (esize == 32) {
            code.psubd(result, arg);
        } else {
            code.psubq(result, arg);
        }
    }

    code.pxor(tmp, result);
    code.pxor(xmm0, arg);
    if constexpr (op == Op::Add) {
        code.pandn(xmm0, tmp);
    } else {
        code.pand(xmm0, tmp);
    }

    code.movaps(tmp, result);
    code.psrad(tmp, 31);
    if constexpr (esize == 64) {
        code.pshufd(tmp, tmp, 0b11110101);
    }
    code.pxor(tmp, code.MConst(xword, msb_mask, msb_mask));

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.ptest(xmm0, code.MConst(xword, msb_mask, msb_mask));
    } else {
        if constexpr (esize == 32) {
            code.movmskps(overflow.cvt32(), xmm0);
        } else {
            code.movmskpd(overflow.cvt32(), xmm0);
        }
        code.test(overflow.cvt32(), overflow.cvt32());
    }
    code.setnz(overflow);
    code.or_(code.byte[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], overflow);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        if constexpr (esize == 32) {
            code.blendvps(result, tmp);
        } else {
            code.blendvpd(result, tmp);
        }

        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        code.psrad(xmm0, 31);
        if constexpr (esize == 64) {
            code.pshufd(xmm0, xmm0, 0b11110101);
        }

        code.pand(tmp, xmm0);
        code.pandn(xmm0, result);
        code.por(tmp, xmm0);

        ctx.reg_alloc.DefineValue(inst, tmp);
    }
}

} // anonymous namespace

void EmitX64::EmitVectorSignedSaturatedAdd8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSaturatedNative(code, ctx, inst, &Xbyak::CodeGenerator::paddsb, &Xbyak::CodeGenerator::paddb, &Xbyak::CodeGenerator::psubb);
}

void EmitX64::EmitVectorSignedSaturatedAdd16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSaturatedNative(code, ctx, inst, &Xbyak::CodeGenerator::paddsw, &Xbyak::CodeGenerator::paddw, &Xbyak::CodeGenerator::psubw);
}

void EmitX64::EmitVectorSignedSaturatedAdd32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturated<Op::Add, 32>(code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedAdd64(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturated<Op::Add, 64>(code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedSub8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSaturatedNative(code, ctx, inst, &Xbyak::CodeGenerator::psubsb, &Xbyak::CodeGenerator::psubb, &Xbyak::CodeGenerator::psubb);
}

void EmitX64::EmitVectorSignedSaturatedSub16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSaturatedNative(code, ctx, inst, &Xbyak::CodeGenerator::psubsw, &Xbyak::CodeGenerator::psubw, &Xbyak::CodeGenerator::psubw);
}

void EmitX64::EmitVectorSignedSaturatedSub32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturated<Op::Sub, 32>(code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedSub64(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturated<Op::Sub, 64>(code, ctx, inst);
}

void EmitX64::EmitVectorUnsignedSaturatedAdd8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSaturatedNative(code, ctx, inst, &Xbyak::CodeGenerator::paddusb, &Xbyak::CodeGenerator::paddb, &Xbyak::CodeGenerator::psubb);
}

void EmitX64::EmitVectorUnsignedSaturatedAdd16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSaturatedNative(code, ctx, inst, &Xbyak::CodeGenerator::paddusw, &Xbyak::CodeGenerator::paddw, &Xbyak::CodeGenerator::psubw);
}

void EmitX64::EmitVectorUnsignedSaturatedAdd32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm addend = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Reg8 overflow = ctx.reg_alloc.ScratchGpr().cvt8();

    // TODO AVX2, AVX-512: vpternlog

    code.movaps(tmp, result);
    code.movaps(xmm0, result);

    code.pxor(xmm0, addend);
    code.pand(tmp, addend);
    code.paddd(result, addend);

    code.psrld(xmm0, 1);
    code.paddd(tmp, xmm0);
    code.psrad(tmp, 31);

    code.por(result, tmp);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.ptest(tmp, tmp);
    } else {
        code.movmskps(overflow.cvt32(), tmp);
        code.test(overflow.cvt32(), overflow.cvt32());
    }
    code.setnz(overflow);
    code.or_(code.byte[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], overflow);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorUnsignedSaturatedAdd64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm addend = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Reg8 overflow = ctx.reg_alloc.ScratchGpr().cvt8();

    // TODO AVX2, AVX-512: vpternlog

    code.movaps(tmp, result);
    code.movaps(xmm0, result);

    code.pxor(xmm0, addend);
    code.pand(tmp, addend);
    code.paddq(result, addend);

    code.psrlq(xmm0, 1);
    code.paddq(tmp, xmm0);
    code.psrad(tmp, 31);
    code.pshufd(tmp, tmp, 0b11110101);

    code.por(result, tmp);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.ptest(tmp, tmp);
    } else {
        code.movmskpd(overflow.cvt32(), tmp);
        code.test(overflow.cvt32(), overflow.cvt32());
    }
    code.setnz(overflow);
    code.or_(code.byte[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], overflow);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorUnsignedSaturatedSub8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSaturatedNative(code, ctx, inst, &Xbyak::CodeGenerator::psubusb, &Xbyak::CodeGenerator::psubb, &Xbyak::CodeGenerator::psubb);
}

void EmitX64::EmitVectorUnsignedSaturatedSub16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSaturatedNative(code, ctx, inst, &Xbyak::CodeGenerator::psubusw, &Xbyak::CodeGenerator::psubw, &Xbyak::CodeGenerator::psubw);
}

void EmitX64::EmitVectorUnsignedSaturatedSub32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm subtrahend = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Reg8 overflow = ctx.reg_alloc.ScratchGpr().cvt8();

    // TODO AVX2, AVX-512: vpternlog

    code.movaps(tmp, result);
    code.movaps(xmm0, subtrahend);

    code.pxor(tmp, subtrahend);
    code.psubd(result, subtrahend);
    code.pand(xmm0, tmp);

    code.psrld(tmp, 1);
    code.psubd(tmp, xmm0);
    code.psrad(tmp, 31);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.ptest(tmp, tmp);
    } else {
        code.movmskps(overflow.cvt32(), tmp);
        code.test(overflow.cvt32(), overflow.cvt32());
    }
    code.setnz(overflow);
    code.or_(code.byte[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], overflow);

    code.pandn(tmp, result);
    ctx.reg_alloc.DefineValue(inst, tmp);
}

void EmitX64::EmitVectorUnsignedSaturatedSub64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm subtrahend = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Reg8 overflow = ctx.reg_alloc.ScratchGpr().cvt8();

    // TODO AVX2, AVX-512: vpternlog

    code.movaps(tmp, result);
    code.movaps(xmm0, subtrahend);

    code.pxor(tmp, subtrahend);
    code.psubq(result, subtrahend);
    code.pand(xmm0, tmp);

    code.psrlq(tmp, 1);
    code.psubq(tmp, xmm0);
    code.psrad(tmp, 31);
    code.pshufd(tmp, tmp, 0b11110101);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.ptest(tmp, tmp);
    } else {
        code.movmskpd(overflow.cvt32(), tmp);
        code.test(overflow.cvt32(), overflow.cvt32());
    }
    code.setnz(overflow);
    code.or_(code.byte[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], overflow);

    code.pandn(tmp, result);
    ctx.reg_alloc.DefineValue(inst, tmp);
}

} // namespace Dynarmic::Backend::X64
