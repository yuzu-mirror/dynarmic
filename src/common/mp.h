/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include "common/common_types.h"

namespace Dynarmic {
namespace mp {

template<typename MemFnT>
struct MemFnInfo;

/// This struct provides information about a member function pointer.
template<typename T, typename ReturnT, typename ...Args>
struct MemFnInfo<ReturnT (T::*)(Args...)> {
    using class_type = T;
    using return_type = ReturnT;
    static constexpr size_t args_count = sizeof...(Args);
};

} // namespace mp
} // namespace Dynarmic
