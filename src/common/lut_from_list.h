/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <map>

#include <mp/typelist/list.h>

namespace Dynarmic::Common {

template <typename KeyT, typename ValueT, typename Function, typename ...Values>
inline auto GenerateLookupTableFromList(Function f, mp::list<Values...>) {
    static const std::array<std::pair<KeyT, ValueT>, sizeof...(Values)> pair_array{f(Values{})...};
    return std::map<KeyT, ValueT>(pair_array.begin(), pair_array.end());
}

} // namespace Dynarmic::Common
