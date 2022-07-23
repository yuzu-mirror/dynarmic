/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/arm64/abi.h"

#include <vector>

#include <mcl/bit/bit_field.hpp>
#include <mcl/stdint.hpp>
#include <oaknut/oaknut.hpp>

namespace Dynarmic::Backend::Arm64 {

using namespace oaknut::util;

static constexpr size_t gpr_size = 8;
static constexpr size_t fpr_size = 16;

struct FrameInfo {
    std::vector<int> gprs;
    std::vector<int> fprs;
    size_t frame_size;
    size_t gprs_size;
    size_t fprs_size;
};

static std::vector<int> ListToIndexes(u32 list) {
    std::vector<int> indexes;
    for (int i = 0; i < 32; i++) {
        if (mcl::bit::get_bit(i, list)) {
            indexes.emplace_back(i);
        }
    }
    return indexes;
}

static FrameInfo CalculateFrameInfo(RegisterList rl, size_t frame_size) {
    const auto gprs = ListToIndexes(static_cast<u32>(rl));
    const auto fprs = ListToIndexes(static_cast<u32>(rl >> 32));

    const size_t num_gprs = gprs.size();
    const size_t num_fprs = fprs.size();

    const size_t gprs_size = (num_gprs + 1) / 2 * 16;
    const size_t fprs_size = num_fprs * 16;

    return {
        gprs,
        fprs,
        frame_size,
        gprs_size,
        fprs_size,
    };
}

void ABI_PushRegisters(oaknut::CodeGenerator& code, RegisterList rl, size_t frame_size) {
    const FrameInfo frame_info = CalculateFrameInfo(rl, frame_size);

    code.SUB(SP, SP, frame_info.gprs_size + frame_info.fprs_size);

    for (size_t i = 0; i < frame_info.gprs.size() - 1; i += 2) {
        code.STP(oaknut::XReg{frame_info.gprs[i]}, oaknut::XReg{frame_info.gprs[i + 1]}, SP, i * gpr_size);
    }
    if (frame_info.gprs.size() % 2 == 1) {
        const size_t i = frame_info.gprs.size() - 1;
        code.STR(oaknut::XReg{frame_info.gprs[i]}, SP, i * gpr_size);
    }

    for (size_t i = 0; i < frame_info.fprs.size() - 1; i += 2) {
        code.STP(oaknut::QReg{frame_info.fprs[i]}, oaknut::QReg{frame_info.fprs[i + 1]}, SP, frame_info.gprs_size + i * fpr_size);
    }
    if (frame_info.fprs.size() % 2 == 1) {
        const size_t i = frame_info.fprs.size() - 1;
        code.STR(oaknut::QReg{frame_info.fprs[i]}, SP, frame_info.gprs_size + i * fpr_size);
    }

    code.SUB(SP, SP, frame_info.frame_size);
}

void ABI_PopRegisters(oaknut::CodeGenerator& code, RegisterList rl, size_t frame_size) {
    const FrameInfo frame_info = CalculateFrameInfo(rl, frame_size);

    code.ADD(SP, SP, frame_info.frame_size);

    for (size_t i = 0; i < frame_info.gprs.size() - 1; i += 2) {
        code.LDP(oaknut::XReg{frame_info.gprs[i]}, oaknut::XReg{frame_info.gprs[i + 1]}, SP, i * gpr_size);
    }
    if (frame_info.gprs.size() % 2 == 1) {
        const size_t i = frame_info.gprs.size() - 1;
        code.LDR(oaknut::XReg{frame_info.gprs[i]}, SP, i * gpr_size);
    }

    for (size_t i = 0; i < frame_info.fprs.size() - 1; i += 2) {
        code.LDP(oaknut::QReg{frame_info.fprs[i]}, oaknut::QReg{frame_info.fprs[i + 1]}, SP, frame_info.gprs_size + i * fpr_size);
    }
    if (frame_info.fprs.size() % 2 == 1) {
        const size_t i = frame_info.fprs.size() - 1;
        code.LDR(oaknut::QReg{frame_info.fprs[i]}, SP, frame_info.gprs_size + i * fpr_size);
    }

    code.ADD(SP, SP, frame_info.gprs_size + frame_info.fprs_size);
}

}  // namespace Dynarmic::Backend::Arm64
