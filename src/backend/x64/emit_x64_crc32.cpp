/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>
#include <climits>

#include "backend/x64/block_of_code.h"
#include "backend/x64/emit_x64.h"
#include "common/crypto/crc32.h"
#include "frontend/ir/microinstruction.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;
namespace CRC32 = Common::Crypto::CRC32;

static void EmitCRC32Castagnoli(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, const int data_size) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.DoesCpuSupport(Xbyak::util::Cpu::tSSE42)) {
        const Xbyak::Reg32 crc = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg value = ctx.reg_alloc.UseGpr(args[1]).changeBit(data_size);
        code.crc32(crc, value);
        ctx.reg_alloc.DefineValue(inst, crc);
        return;
    }

    ctx.reg_alloc.HostCall(inst, args[0], args[1], {});
    code.mov(code.ABI_PARAM3, data_size / CHAR_BIT);
    code.CallFunction(&CRC32::ComputeCRC32Castagnoli);
}

static void EmitCRC32ISO(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, const int data_size) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.DoesCpuSupport(Xbyak::util::Cpu::tPCLMULQDQ) && data_size <= 32) {
        const Xbyak::Reg32 crc = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg64 value = ctx.reg_alloc.UseGpr(args[1]);
        const Xbyak::Xmm xmm_crc = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm xmm_value = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm xmm_const = ctx.reg_alloc.ScratchXmm();

        code.movd(xmm_value, value.cvt32());
        code.movd(xmm_crc, crc);
        code.movdqa(xmm_const, code.MConst(xword, 0x00000001'F7011641, 0x00000001'DB710641));

        code.pxor(xmm_value, xmm_crc);
        if (data_size < 32) {
            code.pslld(xmm_value, 32 - data_size);
            code.psllq(xmm_crc, 32 - data_size);
        }

        code.pclmulqdq(xmm_value, xmm_const, 0x00);
        code.pshufd(xmm_value, xmm_value, 0b11111100);
        code.pclmulqdq(xmm_value, xmm_const, 0x10);

        if (data_size < 32) {
            code.pxor(xmm_value, xmm_crc);
        }

        code.pextrd(crc, xmm_value, 1);

        ctx.reg_alloc.DefineValue(inst, crc);
        return;
    }

    if (code.DoesCpuSupport(Xbyak::util::Cpu::tPCLMULQDQ) && data_size == 64) {
        const Xbyak::Reg32 crc = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg64 value = ctx.reg_alloc.UseScratchGpr(args[1]);
        const Xbyak::Xmm xmm_crc = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm xmm_value = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm xmm_const = ctx.reg_alloc.ScratchXmm();

        code.movd(xmm_value, value.cvt32());
        code.movd(xmm_crc, crc);
        code.movdqa(xmm_const, code.MConst(xword, 0x00000001'F7011641, 0x00000001'DB710641));

        code.pxor(xmm_value, xmm_crc);

        code.pclmulqdq(xmm_value, xmm_const, 0x00);
        code.pshufd(xmm_value, xmm_value, 0b11111100);
        code.pclmulqdq(xmm_value, xmm_const, 0x10);

        code.movq(xmm_crc, value);
        code.pxor(xmm_value, xmm_crc);
        code.pshufd(xmm_value, xmm_value, 0b11111101);

        code.pclmulqdq(xmm_value, xmm_const, 0x00);
        code.pshufd(xmm_value, xmm_value, 0b11111100);
        code.pclmulqdq(xmm_value, xmm_const, 0x10);

        code.pextrd(crc, xmm_value, 1);

        ctx.reg_alloc.DefineValue(inst, crc);
        return;
    }

    ctx.reg_alloc.HostCall(inst, args[0], args[1], {});
    code.mov(code.ABI_PARAM3, data_size / CHAR_BIT);
    code.CallFunction(&CRC32::ComputeCRC32ISO);
}

void EmitX64::EmitCRC32Castagnoli8(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32Castagnoli(code, ctx, inst, 8);
}

void EmitX64::EmitCRC32Castagnoli16(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32Castagnoli(code, ctx, inst, 16);
}

void EmitX64::EmitCRC32Castagnoli32(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32Castagnoli(code, ctx, inst, 32);
}

void EmitX64::EmitCRC32Castagnoli64(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32Castagnoli(code, ctx, inst, 64);
}

void EmitX64::EmitCRC32ISO8(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32ISO(code, ctx, inst, 8);
}

void EmitX64::EmitCRC32ISO16(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32ISO(code, ctx, inst, 16);
}

void EmitX64::EmitCRC32ISO32(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32ISO(code, ctx, inst, 32);
}

void EmitX64::EmitCRC32ISO64(EmitContext& ctx, IR::Inst* inst) {
    EmitCRC32ISO(code, ctx, inst, 64);
}

} // namespace Dynarmic::Backend::X64
