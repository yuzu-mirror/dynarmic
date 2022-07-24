/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/bit_cast.hpp>
#include <mcl/stdint.hpp>
#include <mcl/type_traits/function_info.hpp>

namespace Dynarmic::Backend::Arm64 {

struct DevirtualizedCall {
    u64 fn_ptr;
    u64 this_ptr;
};

template<auto mfp>
DevirtualizedCall Devirtualize(mcl::class_type<decltype(mfp)>* this_) {
    struct MemberFunctionPointer {
        // Address of non-virtual function or index into vtable.
        u64 ptr;
        // LSB is discriminator for if function is virtual. Other bits are this adjustment.
        u64 adj;
    } mfp_struct = mcl::bit_cast<MemberFunctionPointer>(mfp);

    static_assert(sizeof(MemberFunctionPointer) == 16);
    static_assert(sizeof(MemberFunctionPointer) == sizeof(mfp));

    u64 fn_ptr = mfp_struct.ptr;
    u64 this_ptr = mcl::bit_cast<u64>(this_) + (mfp_struct.adj >> 1);
    if (mfp_struct.adj & 1) {
        u64 vtable = mcl::bit_cast_pointee<u64>(this_ptr);
        fn_ptr = mcl::bit_cast_pointee<u64>(vtable + fn_ptr);
    }
    return {fn_ptr, this_ptr};
}

}  // namespace Dynarmic::Backend::Arm64
