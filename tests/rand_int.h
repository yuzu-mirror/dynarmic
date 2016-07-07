// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <random>
#include <type_traits>

template <typename T>
T RandInt(T min, T max) {
    static_assert(std::is_integral<T>::value, "T must be an integral type.");
    static_assert(!std::is_same<T, signed char>::value && !std::is_same<T, unsigned char>::value,
                  "Using char with uniform_int_distribution is undefined behavior.");

    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_int_distribution<T> rand(min, max);
    return rand(mt);
}
