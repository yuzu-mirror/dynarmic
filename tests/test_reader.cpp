/* This file is part of the dynarmic project.
 * Copyright (c) 2023 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <mcl/stdint.hpp>

#include "./A64/testenv.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/interface/A64/a64.h"

using namespace Dynarmic;

A64::UserConfig GetA64UserConfig(A64TestEnv& jit_env) {
    A64::UserConfig jit_user_config{&jit_env};
    jit_user_config.optimizations &= ~OptimizationFlag::FastDispatch;
    // The below corresponds to the settings for qemu's aarch64_max_initfn
    jit_user_config.dczid_el0 = 7;
    jit_user_config.ctr_el0 = 0x80038003;
    jit_user_config.very_verbose_debugging_output = true;
    return jit_user_config;
}

template<size_t num_jit_reruns = 1>
void RunTestInstance(A64::Jit& jit,
                     A64TestEnv& jit_env,
                     const std::array<u64, 31>& regs,
                     const std::array<std::array<u64, 2>, 32>& vecs,
                     const std::vector<u32>& instructions,
                     const u32 pstate,
                     const u32 fpcr,
                     const u64 initial_sp,
                     const u64 start_address,
                     const size_t ticks_left) {
    jit.ClearCache();

    for (size_t jit_rerun_count = 0; jit_rerun_count < num_jit_reruns; ++jit_rerun_count) {
        jit_env.code_mem = instructions;
        jit_env.code_mem.emplace_back(0x14000000);  // B .
        jit_env.code_mem_start_address = start_address;
        jit_env.modified_memory.clear();
        jit_env.interrupts.clear();

        jit.SetRegisters(regs);
        jit.SetVectors(vecs);
        jit.SetPC(start_address);
        jit.SetSP(initial_sp);
        jit.SetFpcr(fpcr);
        jit.SetFpsr(0);
        jit.SetPstate(pstate);
        jit.ClearCache();

        jit_env.ticks_left = ticks_left;
        jit.Run();
    }

    fmt::print("instructions:");
    for (u32 instruction : instructions) {
        fmt::print(" {:08x}", instruction);
    }
    fmt::print("\n");

    fmt::print("initial_regs:");
    for (u64 i : regs) {
        fmt::print(" {:016x}", i);
    }
    fmt::print("\n");
    fmt::print("initial_vecs:");
    for (auto i : vecs) {
        fmt::print(" {:016x}:{:016x}", i[0], i[1]);
    }
    fmt::print("\n");
    fmt::print("initial_sp: {:016x}\n", initial_sp);
    fmt::print("initial_pstate: {:08x}\n", pstate);
    fmt::print("initial_fpcr: {:08x}\n", fpcr);

    fmt::print("final_regs:");
    for (u64 i : jit.GetRegisters()) {
        fmt::print(" {:016x}", i);
    }
    fmt::print("\n");
    fmt::print("final_vecs:");
    for (auto i : jit.GetVectors()) {
        fmt::print(" {:016x}:{:016x}", i[0], i[1]);
    }
    fmt::print("\n");
    fmt::print("final_sp: {:016x}\n", jit.GetSP());
    fmt::print("final_pc: {:016x}\n", jit.GetPC());
    fmt::print("final_pstate: {:08x}\n", jit.GetPstate());
    fmt::print("final_fpcr: {:08x}\n", jit.GetFpcr());
    fmt::print("final_qc : {}\n", FP::FPSR{jit.GetFpsr()}.QC());

    fmt::print("mod_mem:");
    for (auto [addr, value] : jit_env.modified_memory) {
        fmt::print(" {:08x}:{:02x}", addr, value);
    }
    fmt::print("\n");

    fmt::print("interrupts:\n");
    for (const auto& i : jit_env.interrupts) {
        std::puts(i.c_str());
    }

    fmt::print("===\n");
}

int main() {
    std::array<u64, 31> initial_regs{};
    std::array<std::array<u64, 2>, 32> initial_vecs{};
    std::vector<u32> instructions{};
    u32 initial_pstate = 0;
    u32 initial_fpcr = 0;
    u64 initial_sp = 0;
    u64 start_address = 100;

    std::string line;
    while (std::getline(std::cin, line)) {
        std::string_view sv{line};

        const auto skip_ws = [&] {
            auto nextpos{sv.find_first_not_of(' ')};
            if (nextpos != std::string::npos) {
                sv.remove_prefix(nextpos);
            }
        };
        const auto skip_header = [&] {
            sv.remove_prefix(sv.find_first_of(':') + 1);
            skip_ws();
        };
        const auto next_token = [&] {
            auto nextpos{sv.find_first_of(' ')};
            auto tok{sv.substr(0, nextpos)};
            sv.remove_prefix(nextpos == std::string::npos ? sv.size() : nextpos);
            skip_ws();
            return tok;
        };
        const auto parse_hex = [&](std::string_view hex) {
            u64 result = 0;
            while (!hex.empty()) {
                result <<= 4;
                if (hex.front() >= '0' && hex.front() <= '9') {
                    result += hex.front() - '0';
                } else if (hex.front() >= 'a' && hex.front() <= 'f') {
                    result += hex.front() - 'a' + 0xA;
                } else if (hex.front() >= 'A' && hex.front() <= 'F') {
                    result += hex.front() - 'A' + 0xA;
                } else if (hex.front() == ':') {
                    return result;
                } else {
                    fmt::print("Character {} is not a valid hex character\n", hex.front());
                }
                hex.remove_prefix(1);
            }
            return result;
        };

        if (sv.starts_with("instructions:")) {
            skip_header();
            while (!sv.empty()) {
                instructions.emplace_back((u32)parse_hex(next_token()));
            }
        } else if (sv.starts_with("initial_regs:")) {
            skip_header();
            for (size_t i = 0; i < initial_regs.size(); ++i) {
                initial_regs[i] = parse_hex(next_token());
            }
        } else if (sv.starts_with("initial_vecs:")) {
            skip_header();
            for (size_t i = 0; i < initial_vecs.size(); ++i) {
                auto tok{next_token()};
                initial_vecs[i][0] = parse_hex(tok);
                tok.remove_prefix(tok.find_first_of(':') + 1);
                initial_vecs[i][1] = parse_hex(tok);
            }
        } else if (sv.starts_with("initial_sp:")) {
            skip_header();
            initial_sp = parse_hex(next_token());
        } else if (sv.starts_with("initial_pstate:")) {
            skip_header();
            initial_pstate = (u32)parse_hex(next_token());
        } else if (sv.starts_with("initial_fpcr:")) {
            skip_header();
            initial_fpcr = (u32)parse_hex(next_token());
        }
    }

    A64TestEnv jit_env{};
    A64::Jit jit{GetA64UserConfig(jit_env)};
    RunTestInstance(jit,
                    jit_env,
                    initial_regs,
                    initial_vecs,
                    instructions,
                    initial_pstate,
                    initial_fpcr,
                    initial_sp,
                    start_address,
                    instructions.size());

    return 0;
}
