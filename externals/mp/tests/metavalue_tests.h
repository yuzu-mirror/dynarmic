/* This file is part of the mp project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <type_traits>

#include <mp/metavalue/lift_value.h>
#include <mp/metavalue/value_cast.h>
#include <mp/metavalue/value_equal.h>

using namespace mp;

// lift_value

static_assert(std::is_same_v<lift_value<3>, std::integral_constant<int, 3>>);
static_assert(std::is_same_v<lift_value<false>, std::false_type>);

// value_cast

static_assert(std::is_same_v<value_cast<int, std::true_type>, std::integral_constant<int, 1>>);

// value_equal

static_assert(std::is_same_v<value_equal<std::true_type, std::integral_constant<int, 1>>, std::true_type>);
