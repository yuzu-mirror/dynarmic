/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "backend_x64/jitstate.h"
#include "common/assert.h"
#include "common/bit_util.h"
#include "common/common_types.h"

namespace Dynarmic {
namespace BackendX64 {

/**
 * Comparing MXCSR and FPSCR
 * =========================
 *
 * SSE MXCSR exception flags
 * -------------------------
 * PE   bit 5   Precision Flag
 * UE   bit 4   Underflow Flag
 * OE   bit 3   Overflow Flag
 * ZE   bit 2   Divide By Zero Flag
 * DE   bit 1   Denormal Flag
 * IE   bit 0   Invalid Operation Flag
 *
 * VFP FPSCR cumulative exception bits
 * -----------------------------------
 * IDC  bit 7   Input Denormal cumulative exception bit
 * IXC  bit 4   Inexact cumulative exception bit
 * UFC  bit 3   Underflow cumulative exception bit
 * OFC  bit 2   Overflow cumulative exception bit
 * DZC  bit 1   Division by Zero cumulative exception bit
 * IOC  bit 0   Invalid Operation cumulative exception bit
 *
 * SSE MSCSR exception masks
 * -------------------------
 * PM   bit 12  Precision Mask
 * UM   bit 11  Underflow Mask
 * OM   bit 10  Overflow Mask
 * ZM   bit 9   Divide By Zero Mask
 * DM   bit 8   Denormal Mask
 * IM   bit 7   Invalid Operation Mask
 *
 * VFP FPSCR exception trap enables
 * --------------------------------
 * IDE  bit 15  Input Denormal exception trap enable
 * IXE  bit 12  Inexact exception trap enable
 * UFE  bit 11  Underflow exception trap enable
 * OFE  bit 10  Overflow exception trap enable
 * DZE  bit 9   Division by Zero exception trap enable
 * IOE  bit 8   Invalid Operation exception trap enable
 *
 * SSE MXCSR mode bits
 * -------------------
 * FZ   bit 15  Flush To Zero
 * DAZ  bit 6   Denormals Are Zero
 * RN   bits 13-14  Round to {0 = Nearest, 1 = Negative, 2 = Positive, 3 = Zero}
 *
 * VFP FPSCR mode bits
 * -------------------
 * DN   bit 25  Default NaN
 * FZ   bit 24  Flush to Zero
 * RMode    bits 22-23  Round to {0 = Nearest, 1 = Positive, 2 = Negative, 3 = Zero}
 * Stride   bits 20-21  Vector stride
 * Len  bits 16-18  Vector length
 */

// NZCV; QC (ASMID only), AHP; DN, FZ, RMode, Stride; SBZP; Len; trap enables; cumulative bits
constexpr u32 FPSCR_MASK = 0b1111'00'111111'0'111'10011111'00000000;

u32 JitState::Fpscr() const {
    ASSERT((guest_FPSCR_flags & ~FPSCR_MASK) == 0);

    u32 FPSCR = guest_FPSCR_flags;
    FPSCR |= (guest_MXCSR & 0b0000000000001);       // IOC = IE
    FPSCR |= (guest_MXCSR & 0b0000000000010) << 6;  // IDC = DE
    FPSCR |= (guest_MXCSR & 0b0000000111100) >> 1;  // IXC, UFC, OFC, DZC = PE, UE, OE, ZE

    if (!Common::Bit<24>(FPSCR)) {
        // ARM only sets IDC if FTZ == 1.
        FPSCR &= ~(1 << 7);
        FPSCR |= old_FPSCR & (1 << 7);
    }

    return FPSCR;
}

void JitState::SetFpscr(u32 FPSCR) {
    old_FPSCR = FPSCR;

    guest_FPSCR_flags = FPSCR & FPSCR_MASK;

    guest_MXCSR = 0;

    guest_MXCSR |= ( FPSCR     ) & 0b0000000000001;  // IE = IOC
    guest_MXCSR |= ( FPSCR >> 6) & 0b0000000000010;  // DE = IDC
    guest_MXCSR |= ( FPSCR << 1) & 0b0000000111100;  // PE, UE, OE, ZE = IXC, UFC, OFC, DZC
    guest_MXCSR |= (~FPSCR >> 1) & 0b0000010000000;  // IM = ~IOE
    guest_MXCSR |= (~FPSCR >> 7) & 0b0000100000000;  // DM = ~IDE
    guest_MXCSR |= (~FPSCR     ) & 0b1111000000000;  // PM, UM, OM, ZM = ~IXE, ~UFE, ~OFE, ~DZE

    if (Common::Bit<24>(FPSCR)) {
        // VFP Flush to Zero
        guest_MXCSR |= (1 << 15); // SSE Flush to Zero
        guest_MXCSR |= (1 << 6);  // SSE Denormals are Zero
    }
}


} // namespace BackendX64
} // namespace Dynarmic
