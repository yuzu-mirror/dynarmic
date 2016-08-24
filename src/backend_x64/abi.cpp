// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// 24th August 2016: This code was modified for Dynarmic.

#include <xbyak.h>

#include "backend_x64/abi.h"
#include "common/common_types.h"
#include "common/iterator_util.h"

namespace Dynarmic {
namespace BackendX64 {

constexpr size_t GPR_SIZE = 8;
constexpr size_t XMM_SIZE = 16;

struct FrameInfo {
    size_t stack_subtraction = 0;
    size_t xmm_offset = 0;
};

static FrameInfo CalculateFrameInfo(size_t num_gprs, size_t num_xmms, size_t frame_size) {
    FrameInfo frame_info = {};

    size_t rsp_alignment = 8; // We are always 8-byte aligned initially
    rsp_alignment -= num_gprs * GPR_SIZE;

    if (num_xmms > 0) {
        frame_info.stack_subtraction = rsp_alignment & 0xF;
        frame_info.stack_subtraction += num_xmms * XMM_SIZE;
    }

    size_t xmm_base = frame_info.stack_subtraction;

    frame_info.stack_subtraction += frame_size;
    frame_info.stack_subtraction += ABI_SHADOW_SPACE;

    rsp_alignment -= frame_info.stack_subtraction;
    frame_info.stack_subtraction += rsp_alignment & 0xF;

    frame_info.xmm_offset = frame_info.stack_subtraction - xmm_base;

    return frame_info;
}

void ABI_PushCalleeSaveRegistersAndAdjustStack(Xbyak::CodeGenerator* code, size_t frame_size) {
    using namespace Xbyak::util;

    const size_t num_gprs = std::count_if(ABI_ALL_CALLEE_SAVE.begin(), ABI_ALL_CALLEE_SAVE.end(), HostLocIsGPR);
    const size_t num_xmms = std::count_if(ABI_ALL_CALLEE_SAVE.begin(), ABI_ALL_CALLEE_SAVE.end(), HostLocIsXMM);

    FrameInfo frame_info = CalculateFrameInfo(num_gprs, num_xmms, frame_size);

    for (HostLoc gpr : ABI_ALL_CALLEE_SAVE) {
        if (HostLocIsGPR(gpr)) {
            code->push(HostLocToReg64(gpr));
        }
    }

    if (frame_info.stack_subtraction != 0) {
        code->sub(rsp, u32(frame_info.stack_subtraction));
    }

    size_t xmm_offset = frame_info.xmm_offset;
    for (HostLoc xmm : ABI_ALL_CALLEE_SAVE) {
        if (HostLocIsXMM(xmm)) {
            code->movaps(code->xword[rsp + xmm_offset], HostLocToXmm(xmm));
            xmm_offset += XMM_SIZE;
        }
    }
}

void ABI_PopCalleeSaveRegistersAndAdjustStack(Xbyak::CodeGenerator* code, size_t frame_size) {
    using namespace Xbyak::util;

    const size_t num_gprs = std::count_if(ABI_ALL_CALLEE_SAVE.begin(), ABI_ALL_CALLEE_SAVE.end(), HostLocIsGPR);
    const size_t num_xmms = std::count_if(ABI_ALL_CALLEE_SAVE.begin(), ABI_ALL_CALLEE_SAVE.end(), HostLocIsXMM);

    FrameInfo frame_info = CalculateFrameInfo(num_gprs, num_xmms, frame_size);

    size_t xmm_offset = frame_info.xmm_offset;
    for (HostLoc xmm : Common::Reverse(ABI_ALL_CALLEE_SAVE)) {
        if (HostLocIsXMM(xmm)) {
            code->movaps(HostLocToXmm(xmm), code->xword[rsp + xmm_offset]);
            xmm_offset += XMM_SIZE;
        }
    }

    if (frame_info.stack_subtraction != 0) {
        code->add(rsp, u32(frame_info.stack_subtraction));
    }

    for (HostLoc gpr : Common::Reverse(ABI_ALL_CALLEE_SAVE)) {
        if (HostLocIsGPR(gpr)) {
            code->pop(HostLocToReg64(gpr));
        }
    }
}

} // namespace BackendX64
} // namespace Dynarmic
