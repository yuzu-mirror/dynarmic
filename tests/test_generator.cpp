/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <algorithm>
#include <array>
#include <cstdio>
#include <functional>
#include <tuple>
#include <vector>

#include <mcl/bit/swap.hpp>
#include <mcl/stdint.hpp>

#include "./A32/testenv.h"
#include "./fuzz_util.h"
#include "./rand_int.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/frontend/A32/ITState.h"
#include "dynarmic/frontend/A32/a32_location_descriptor.h"
#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/frontend/A32/translate/a32_translate.h"
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/location_descriptor.h"
#include "dynarmic/ir/opcodes.h"

// Must be declared last for all necessary operator<< to be declared prior to this.
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace {
using namespace Dynarmic;

bool ShouldTestInst(u32 instruction, u32 pc, bool is_thumb, bool is_last_inst, A32::ITState it_state = {}) {
    const A32::LocationDescriptor location = A32::LocationDescriptor{pc, {}, {}}.SetTFlag(is_thumb).SetIT(it_state);
    IR::Block block{location};
    const bool should_continue = A32::TranslateSingleInstruction(block, location, instruction);

    if (!should_continue && !is_last_inst) {
        return false;
    }

    if (auto terminal = block.GetTerminal(); boost::get<IR::Term::Interpret>(&terminal)) {
        return false;
    }

    for (const auto& ir_inst : block) {
        switch (ir_inst.GetOpcode()) {
        case IR::Opcode::A32ExceptionRaised:
        case IR::Opcode::A32CallSupervisor:
        case IR::Opcode::A32CoprocInternalOperation:
        case IR::Opcode::A32CoprocSendOneWord:
        case IR::Opcode::A32CoprocSendTwoWords:
        case IR::Opcode::A32CoprocGetOneWord:
        case IR::Opcode::A32CoprocGetTwoWords:
        case IR::Opcode::A32CoprocLoadWords:
        case IR::Opcode::A32CoprocStoreWords:
            return false;
        default:
            continue;
        }
    }

    return true;
}

u32 GenRandomArmInst(u32 pc, bool is_last_inst) {
    static const struct InstructionGeneratorInfo {
        std::vector<InstructionGenerator> generators;
        std::vector<InstructionGenerator> invalid;
    } instructions = [] {
        const std::vector<std::tuple<std::string, const char*>> list{
#define INST(fn, name, bitstring) {#fn, bitstring},
#include "dynarmic/frontend/A32/decoder/arm.inc"
//#include "dynarmic/frontend/A32/decoder/asimd.inc"
//#include "dynarmic/frontend/A32/decoder/vfp.inc"
#undef INST
        };

        std::vector<InstructionGenerator> generators;
        std::vector<InstructionGenerator> invalid;

        // List of instructions not to test
        static constexpr std::array do_not_test{
            // Translating load/stores
            "arm_LDRBT", "arm_LDRBT", "arm_LDRHT", "arm_LDRHT", "arm_LDRSBT", "arm_LDRSBT", "arm_LDRSHT", "arm_LDRSHT", "arm_LDRT", "arm_LDRT",
            "arm_STRBT", "arm_STRBT", "arm_STRHT", "arm_STRHT", "arm_STRT", "arm_STRT",
            // Exclusive load/stores
            "arm_LDREXB", "arm_LDREXD", "arm_LDREXH", "arm_LDREX", "arm_LDAEXB", "arm_LDAEXD", "arm_LDAEXH", "arm_LDAEX",
            "arm_STREXB", "arm_STREXD", "arm_STREXH", "arm_STREX", "arm_STLEXB", "arm_STLEXD", "arm_STLEXH", "arm_STLEX",
            "arm_SWP", "arm_SWPB",
            // Elevated load/store multiple instructions.
            "arm_LDM_eret", "arm_LDM_usr",
            "arm_STM_usr",
            // Coprocessor
            "arm_CDP", "arm_LDC", "arm_MCR", "arm_MCRR", "arm_MRC", "arm_MRRC", "arm_STC",
            // System
            "arm_CPS", "arm_RFE", "arm_SRS",
            // Undefined
            "arm_UDF",
            // FPSCR is inaccurate
            "vfp_VMRS",
            // Incorrect Unicorn implementations
            "asimd_VRECPS",         // Unicorn does not fuse the multiply and subtraction, resulting in being off by 1ULP.
            "asimd_VRSQRTS",        // Unicorn does not fuse the multiply and subtraction, resulting in being off by 1ULP.
            "vfp_VCVT_from_fixed",  // Unicorn does not do round-to-nearest-even for this instruction correctly.
        };

        for (const auto& [fn, bitstring] : list) {
            if (std::find(do_not_test.begin(), do_not_test.end(), fn) != do_not_test.end()) {
                invalid.emplace_back(InstructionGenerator{bitstring});
                continue;
            }
            generators.emplace_back(InstructionGenerator{bitstring});
        }
        return InstructionGeneratorInfo{generators, invalid};
    }();

    while (true) {
        const size_t index = RandInt<size_t>(0, instructions.generators.size() - 1);
        const u32 inst = instructions.generators[index].Generate();

        if ((instructions.generators[index].Mask() & 0xF0000000) == 0 && (inst & 0xF0000000) == 0xF0000000) {
            continue;
        }

        if (ShouldTestInst(inst, pc, false, is_last_inst)) {
            return inst;
        }
    }
}

std::vector<u16> GenRandomThumbInst(u32 pc, bool is_last_inst, A32::ITState it_state = {}) {
    static const struct InstructionGeneratorInfo {
        std::vector<InstructionGenerator> generators;
        std::vector<InstructionGenerator> invalid;
    } instructions = [] {
        const std::vector<std::tuple<std::string, const char*>> list{
#define INST(fn, name, bitstring) {#fn, bitstring},
#include "dynarmic/frontend/A32/decoder/thumb16.inc"
#include "dynarmic/frontend/A32/decoder/thumb32.inc"
#undef INST
        };

        const std::vector<std::tuple<std::string, const char*>> vfp_list{
#define INST(fn, name, bitstring) {#fn, bitstring},
//#include "dynarmic/frontend/A32/decoder/vfp.inc"
#undef INST
        };

        const std::vector<std::tuple<std::string, const char*>> asimd_list{
#define INST(fn, name, bitstring) {#fn, bitstring},
//#include "dynarmic/frontend/A32/decoder/asimd.inc"
#undef INST
        };

        std::vector<InstructionGenerator> generators;
        std::vector<InstructionGenerator> invalid;

        // List of instructions not to test
        static constexpr std::array do_not_test{
            "thumb16_BKPT",
            "thumb16_IT",

            // Exclusive load/stores
            "thumb32_LDREX",
            "thumb32_LDREXB",
            "thumb32_LDREXD",
            "thumb32_LDREXH",
            "thumb32_STREX",
            "thumb32_STREXB",
            "thumb32_STREXD",
            "thumb32_STREXH",

            // Coprocessor
            "thumb32_CDP",
            "thumb32_LDC",
            "thumb32_MCR",
            "thumb32_MCRR",
            "thumb32_MRC",
            "thumb32_MRRC",
            "thumb32_STC",
        };

        for (const auto& [fn, bitstring] : list) {
            if (std::find(do_not_test.begin(), do_not_test.end(), fn) != do_not_test.end()) {
                invalid.emplace_back(InstructionGenerator{bitstring});
                continue;
            }
            generators.emplace_back(InstructionGenerator{bitstring});
        }
        for (const auto& [fn, bs] : vfp_list) {
            std::string bitstring = bs;
            if (bitstring.substr(0, 4) == "cccc" || bitstring.substr(0, 4) == "----") {
                bitstring.replace(0, 4, "1110");
            }
            if (std::find(do_not_test.begin(), do_not_test.end(), fn) != do_not_test.end()) {
                invalid.emplace_back(InstructionGenerator{bitstring.c_str()});
                continue;
            }
            generators.emplace_back(InstructionGenerator{bitstring.c_str()});
        }
        for (const auto& [fn, bs] : asimd_list) {
            std::string bitstring = bs;
            if (bitstring.substr(0, 7) == "1111001") {
                const char U = bitstring[7];
                bitstring.replace(0, 8, "111-1111");
                bitstring[3] = U;
            } else if (bitstring.substr(0, 8) == "11110100") {
                bitstring.replace(0, 8, "11111001");
            } else {
                ASSERT_FALSE("Unhandled ASIMD instruction: {} {}", fn, bs);
            }
            if (std::find(do_not_test.begin(), do_not_test.end(), fn) != do_not_test.end()) {
                invalid.emplace_back(InstructionGenerator{bitstring.c_str()});
                continue;
            }
            generators.emplace_back(InstructionGenerator{bitstring.c_str()});
        }
        return InstructionGeneratorInfo{generators, invalid};
    }();

    while (true) {
        const size_t index = RandInt<size_t>(0, instructions.generators.size() - 1);
        const u32 inst = instructions.generators[index].Generate();
        const bool is_four_bytes = (inst >> 16) != 0;

        if (ShouldTestInst(is_four_bytes ? mcl::bit::swap_halves_32(inst) : inst, pc, true, is_last_inst, it_state)) {
            if (is_four_bytes)
                return {static_cast<u16>(inst >> 16), static_cast<u16>(inst)};
            return {static_cast<u16>(inst)};
        }
    }
}

template<typename TestEnv>
Dynarmic::A32::UserConfig GetUserConfig(TestEnv& testenv) {
    Dynarmic::A32::UserConfig user_config;
    user_config.optimizations &= ~OptimizationFlag::FastDispatch;
    user_config.callbacks = &testenv;
    return user_config;
}

template<typename TestEnv>
static void RunTestInstance(Dynarmic::A32::Jit& jit,
                            TestEnv& jit_env,
                            const std::array<u32, 16>& regs,
                            const std::array<u32, 64>& vecs,
                            const std::vector<typename TestEnv::InstructionType>& instructions,
                            const u32 cpsr,
                            const u32 fpscr,
                            const size_t ticks_left) {
    const u32 initial_pc = regs[15];
    const u32 num_words = initial_pc / sizeof(typename TestEnv::InstructionType);
    const u32 code_mem_size = num_words + static_cast<u32>(instructions.size());

    jit_env.code_mem.resize(code_mem_size);
    std::fill(jit_env.code_mem.begin(), jit_env.code_mem.end(), TestEnv::infinite_loop);

    std::copy(instructions.begin(), instructions.end(), jit_env.code_mem.begin() + num_words);
    jit_env.PadCodeMem();
    jit_env.modified_memory.clear();
    jit_env.interrupts.clear();

    jit.Regs() = regs;
    jit.ExtRegs() = vecs;
    jit.SetFpscr(fpscr);
    jit.SetCpsr(cpsr);
    jit.ClearCache();

    jit_env.ticks_left = ticks_left;
    jit.Run();

    fmt::print("instructions: ");
    for (auto instruction : instructions) {
        if constexpr (sizeof(decltype(instruction)) == 2) {
            fmt::print("{:04x} ", instruction);
        } else {
            fmt::print("{:08x} ", instruction);
        }
    }
    fmt::print("\n");

    fmt::print("initial_regs: ");
    for (u32 i : regs) {
        fmt::print("{:08x} ", i);
    }
    fmt::print("\n");
    fmt::print("initial_vecs: ");
    for (u32 i : vecs) {
        fmt::print("{:08x} ", i);
    }
    fmt::print("\n");
    fmt::print("initial_cpsr: {:08x}\n", cpsr);
    fmt::print("initial_fpcr: {:08x}\n", fpscr);

    fmt::print("final_regs: ");
    for (u32 i : jit.Regs()) {
        fmt::print("{:08x} ", i);
    }
    fmt::print("\n");
    fmt::print("final_vecs: ");
    for (u32 i : jit.ExtRegs()) {
        fmt::print("{:08x} ", i);
    }
    fmt::print("\n");
    fmt::print("final_cpsr: {:08x}\n", jit.Cpsr());
    fmt::print("final_fpsr: {:08x}\n", jit.Fpscr());

    fmt::print("mod_mem: ");
    for (auto [addr, value] : jit_env.modified_memory) {
        fmt::print("{:08x}:{:02x} ", addr, value);
    }
    fmt::print("\n");

    fmt::print("interrupts:\n");
    for (const auto& i : jit_env.interrupts) {
        std::puts(i.c_str());
    }

    fmt::print("===\n");
}
}  // Anonymous namespace

void TestThumb(size_t num_instructions, size_t num_iterations = 100000) {
    ThumbTestEnv jit_env{};
    Dynarmic::A32::Jit jit{GetUserConfig(jit_env)};

    std::array<u32, 16> regs;
    std::array<u32, 64> ext_reg;
    std::vector<u16> instructions;

    for (size_t iteration = 0; iteration < num_iterations; ++iteration) {
        std::generate(regs.begin(), regs.end(), [] { return RandInt<u32>(0, ~u32(0)); });
        std::generate(ext_reg.begin(), ext_reg.end(), [] { return RandInt<u32>(0, ~u32(0)); });

        const u32 start_address = 100;
        const u32 cpsr = (RandInt<u32>(0, 0xF) << 28) | 0x1F0;
        const u32 fpcr = RandomFpcr();

        instructions.clear();
        for (size_t i = 0; i < num_instructions; ++i) {
            const auto inst = GenRandomThumbInst(static_cast<u32>(start_address + 2 * instructions.size()), i == num_instructions - 1);
            instructions.insert(instructions.end(), inst.begin(), inst.end());
        }

        regs[15] = start_address;
        RunTestInstance(jit, jit_env, regs, ext_reg, instructions, cpsr, fpcr, num_instructions);
    }
}

void TestArm(size_t num_instructions, size_t num_iterations = 100000) {
    ArmTestEnv jit_env{};
    Dynarmic::A32::Jit jit{GetUserConfig(jit_env)};

    std::array<u32, 16> regs;
    std::array<u32, 64> ext_reg;
    std::vector<u32> instructions;

    for (size_t iteration = 0; iteration < num_iterations; ++iteration) {
        std::generate(regs.begin(), regs.end(), [] { return RandInt<u32>(0, ~u32(0)); });
        std::generate(ext_reg.begin(), ext_reg.end(), [] { return RandInt<u32>(0, ~u32(0)); });

        const u32 start_address = 100;
        const u32 cpsr = (RandInt<u32>(0, 0xF) << 28);
        const u32 fpcr = RandomFpcr();

        instructions.clear();
        for (size_t i = 0; i < num_instructions; ++i) {
            instructions.emplace_back(GenRandomArmInst(static_cast<u32>(start_address + 4 * instructions.size()), i == num_instructions - 1));
        }

        regs[15] = start_address;
        RunTestInstance(jit, jit_env, regs, ext_reg, instructions, cpsr, fpcr, 1);
    }
}

int main(int, char*[]) {
    detail::g_rand_int_generator.seed(42069);

    TestThumb(1);
    TestArm(1);
    TestThumb(1024, 1000);
    TestArm(1024, 1000);

    return 0;
}
