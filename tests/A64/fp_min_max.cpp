/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <vector>

#include <catch2/catch.hpp>
#include <mcl/stdint.hpp>

#include "./testenv.h"

using namespace Dynarmic;

namespace {

struct TestCase {
    u32 a;
    u32 b;
    u32 fmax;
    u32 fmaxnm;
    u32 fmin;
    u32 fminnm;
};

const std::vector test_cases{
    //                a           b        fmax      fmaxnm        fmin      fminnm
    TestCase{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},  // +0.0
    TestCase{0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},  // -0.0
    TestCase{0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000},  // +1.0
    TestCase{0xbf800000, 0xbf800000, 0xbf800000, 0xbf800000, 0xbf800000, 0xbf800000},  // -1.0
    TestCase{0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000},  // +Inf
    TestCase{0xff800000, 0xff800000, 0xff800000, 0xff800000, 0xff800000, 0xff800000},  // -Inf
    TestCase{0x7fc00042, 0x7fc00042, 0x7fc00042, 0x7fc00042, 0x7fc00042, 0x7fc00042},  // QNaN
    TestCase{0x7f800042, 0x7f800042, 0x7fc00042, 0x7fc00042, 0x7fc00042, 0x7fc00042},  // SNaN
    TestCase{0x00000000, 0x80000000, 0x00000000, 0x00000000, 0x80000000, 0x80000000},  // (+0.0, -0.0)
    TestCase{0x3f800000, 0xbf800000, 0x3f800000, 0x3f800000, 0xbf800000, 0xbf800000},  // (+1.0, -1.0)
    TestCase{0x3f800000, 0x7f800000, 0x7f800000, 0x7f800000, 0x3f800000, 0x3f800000},  // (+1.0, +Inf)
    TestCase{0x3f800000, 0xff800000, 0x3f800000, 0x3f800000, 0xff800000, 0xff800000},  // (+1.0, -Inf)
    TestCase{0x3f800000, 0x7fc00042, 0x7fc00042, 0x3f800000, 0x7fc00042, 0x3f800000},  // (+1.0, QNaN)
    TestCase{0x3f800000, 0x7f800042, 0x7fc00042, 0x7fc00042, 0x7fc00042, 0x7fc00042},  // (+1.0, SNaN)
};

template<typename Fn>
void run_test(u32 instruction, Fn fn) {
    A64TestEnv env;
    A64::Jit jit{A64::UserConfig{&env}};

    for (const auto test_case : test_cases) {
        env.code_mem.emplace_back(instruction);  // FMAX S0, S1, S2
        env.code_mem.emplace_back(0x14000000);   // B .

        INFO(test_case.a);
        INFO(test_case.b);

        jit.SetVector(0, {42, 0});
        jit.SetVector(1, {test_case.a, 0});
        jit.SetVector(2, {test_case.b, 0});
        jit.SetPC(0);

        env.ticks_left = 2;
        jit.Run();

        REQUIRE(jit.GetVector(0)[0] == fn(test_case));

        jit.SetVector(0, {42, 0});
        jit.SetVector(1, {test_case.b, 0});
        jit.SetVector(2, {test_case.a, 0});
        jit.SetPC(0);

        env.ticks_left = 2;
        jit.Run();

        REQUIRE(jit.GetVector(0)[0] == fn(test_case));
    }
}

}  // namespace

TEST_CASE("A64: FMAX (scalar)", "[a64]") {
    run_test(0x1e224820, [](const TestCase& test_case) { return test_case.fmax; });
}

TEST_CASE("A64: FMIN (scalar)", "[a64]") {
    run_test(0x1e225820, [](const TestCase& test_case) { return test_case.fmin; });
}

TEST_CASE("A64: FMAXNM (scalar)", "[a64]") {
    run_test(0x1e226820, [](const TestCase& test_case) { return test_case.fmaxnm; });
}

TEST_CASE("A64: FMINNM (scalar)", "[a64]") {
    run_test(0x1e227820, [](const TestCase& test_case) { return test_case.fminnm; });
}

TEST_CASE("A64: FMAX (vector)", "[a64]") {
    run_test(0x4e22f420, [](const TestCase& test_case) { return test_case.fmax; });
}

TEST_CASE("A64: FMIN (vector)", "[a64]") {
    run_test(0x4ea2f420, [](const TestCase& test_case) { return test_case.fmin; });
}

TEST_CASE("A64: FMAXNM (vector)", "[a64]") {
    run_test(0x4e22c420, [](const TestCase& test_case) { return test_case.fmaxnm; });
}

TEST_CASE("A64: FMINNM (vector)", "[a64]") {
    run_test(0x4ea2c420, [](const TestCase& test_case) { return test_case.fminnm; });
}
