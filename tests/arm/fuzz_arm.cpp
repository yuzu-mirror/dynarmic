/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <cinttypes>
#include <cstring>
#include <functional>

#include <catch.hpp>

#include "common/bit_util.h"
#include "common/common_types.h"
#include "frontend/disassembler/disassembler.h"
#include "interface/interface.h"
#include "rand_int.h"
#include "skyeye_interpreter/dyncom/arm_dyncom_interpreter.h"
#include "skyeye_interpreter/skyeye_common/armstate.h"

struct WriteRecord {
    size_t size;
    u32 address;
    u64 data;
};

static bool operator==(const WriteRecord& a, const WriteRecord& b) {
    return std::tie(a.size, a.address, a.data) == std::tie(b.size, b.address, b.data);
}

static std::array<u32, 3000> code_mem{};
static std::vector<WriteRecord> write_records;

static bool IsReadOnlyMemory(u32 vaddr);
static u8 MemoryRead8(u32 vaddr);
static u16 MemoryRead16(u32 vaddr);
static u32 MemoryRead32(u32 vaddr);
static u64 MemoryRead64(u32 vaddr);
static void MemoryWrite8(u32 vaddr, u8 value);
static void MemoryWrite16(u32 vaddr, u16 value);
static void MemoryWrite32(u32 vaddr, u32 value);
static void MemoryWrite64(u32 vaddr, u64 value);
static void InterpreterFallback(u32 pc, Dynarmic::Jit* jit);
static Dynarmic::UserCallbacks GetUserCallbacks();

static bool IsReadOnlyMemory(u32 vaddr) {
    return vaddr < code_mem.size();
}
static u8 MemoryRead8(u32 vaddr) {
    return static_cast<u8>(vaddr);
}
static u16 MemoryRead16(u32 vaddr) {
    return static_cast<u16>(vaddr);
}
static u32 MemoryRead32(u32 vaddr) {
    if (vaddr < code_mem.size() * sizeof(u32)) {
        size_t index = vaddr / sizeof(u32);
        return code_mem[index];
    }
    return vaddr;
}
static u64 MemoryRead64(u32 vaddr) {
    return vaddr;
}

static void MemoryWrite8(u32 vaddr, u8 value){
    write_records.push_back({8, vaddr, value});
}
static void MemoryWrite16(u32 vaddr, u16 value){
    write_records.push_back({16, vaddr, value});
}
static void MemoryWrite32(u32 vaddr, u32 value){
    write_records.push_back({32, vaddr, value});
}
static void MemoryWrite64(u32 vaddr, u64 value){
    write_records.push_back({64, vaddr, value});
}

static void InterpreterFallback(u32 pc, Dynarmic::Jit* jit) {
    ARMul_State interp_state{USER32MODE};
    interp_state.user_callbacks = GetUserCallbacks();
    interp_state.NumInstrsToExecute = 1;

    interp_state.Reg = jit->Regs();
    interp_state.Cpsr = jit->Cpsr();
    interp_state.Reg[15] = pc;

    InterpreterClearCache();
    InterpreterMainLoop(&interp_state);

    bool T = Dynarmic::Common::Bit<5>(interp_state.Cpsr);
    interp_state.Reg[15] &= T ? 0xFFFFFFFE : 0xFFFFFFFC;

    jit->Regs() = interp_state.Reg;
    jit->Cpsr() = interp_state.Cpsr;
}

static void Fail() {
    FAIL();
}

static Dynarmic::UserCallbacks GetUserCallbacks() {
    Dynarmic::UserCallbacks user_callbacks{};
    user_callbacks.InterpreterFallback = &InterpreterFallback;
    user_callbacks.CallSVC = (bool (*)(u32)) &Fail;
    user_callbacks.IsReadOnlyMemory = &IsReadOnlyMemory;
    user_callbacks.MemoryRead8 = &MemoryRead8;
    user_callbacks.MemoryRead16 = &MemoryRead16;
    user_callbacks.MemoryRead32 = &MemoryRead32;
    user_callbacks.MemoryRead64 = &MemoryRead64;
    user_callbacks.MemoryWrite8 = &MemoryWrite8;
    user_callbacks.MemoryWrite16 = &MemoryWrite16;
    user_callbacks.MemoryWrite32 = &MemoryWrite32;
    user_callbacks.MemoryWrite64 = &MemoryWrite64;
    return user_callbacks;
}

struct InstructionGenerator final {
public:
    InstructionGenerator(const char* format, std::function<bool(u32)> is_valid = [](u32){ return true; }) : is_valid(is_valid) {
        REQUIRE(strlen(format) == 32);

        for (int i = 0; i < 32; i++) {
            const u32 bit = 1 << (31 - i);
            switch (format[i]) {
                case '0':
                    mask |= bit;
                    break;
                case '1':
                    bits |= bit;
                    mask |= bit;
                    break;
                default:
                    // Do nothing
                    break;
            }
        }
    }
    u32 Generate() const {
        u32 inst;
        do {
            u32 random = RandInt<u32>(0, 0xFFFF);
            inst = bits | (random & ~mask);
        } while (!is_valid(inst));
        return inst;
    }
    u32 Bits() { return bits; }
    u32 Mask() { return mask; }
private:
    u32 bits = 0;
    u32 mask = 0;
    std::function<bool(u32)> is_valid;
};

static bool DoesBehaviorMatch(const ARMul_State& interp, const Dynarmic::Jit& jit, const std::vector<WriteRecord>& interp_write_records, const std::vector<WriteRecord>& jit_write_records) {
    const auto interp_regs = interp.Reg;
    const auto jit_regs = jit.Regs();

    return std::equal(interp_regs.begin(), interp_regs.end(), jit_regs.begin(), jit_regs.end())
           && interp.Cpsr == jit.Cpsr()
           && interp_write_records == jit_write_records;
}


void FuzzJitArm(const size_t instruction_count, const size_t instructions_to_execute_count, const size_t run_count, const std::function<u32()> instruction_generator) {
    // Prepare memory
    code_mem.fill(0xEAFFFFFE); // b +#0

    // Prepare test subjects
    ARMul_State interp{USER32MODE};
    interp.user_callbacks = GetUserCallbacks();
    Dynarmic::Jit jit{GetUserCallbacks()};

    for (size_t run_number = 0; run_number < run_count; run_number++) {
        interp.instruction_cache.clear();
        InterpreterClearCache();
        jit.ClearCache(false);

        // Setup initial state

        std::array<u32, 16> initial_regs;
        std::generate_n(initial_regs.begin(), 15, []{ return RandInt<u32>(0, 0xFFFFFFFF); });
        initial_regs[15] = 0;

        interp.Cpsr = 0x000001D0;
        interp.Reg = initial_regs;
        jit.Cpsr() = 0x000001D0;
        jit.Regs() = initial_regs;

        std::generate_n(code_mem.begin(), instruction_count, instruction_generator);

        // Run interpreter
        write_records.clear();
        interp.NumInstrsToExecute = static_cast<unsigned>(instructions_to_execute_count);
        InterpreterMainLoop(&interp);
        auto interp_write_records = write_records;
        {
            bool T = Dynarmic::Common::Bit<5>(interp.Cpsr);
            interp.Reg[15] &= T ? 0xFFFFFFFE : 0xFFFFFFFC;
        }

        // Run jit
        write_records.clear();
        jit.Run(static_cast<unsigned>(instructions_to_execute_count));
        auto jit_write_records = write_records;

        // Compare
        if (!DoesBehaviorMatch(interp, jit, interp_write_records, jit_write_records)) {
            printf("Failed at execution number %zu\n", run_number);

            printf("\nInstruction Listing: \n");
            for (size_t i = 0; i < instruction_count; i++) {
                 printf("%s\n", Dynarmic::Arm::DisassembleArm(code_mem[i]).c_str());
            }

            printf("\nInitial Register Listing: \n");
            for (int i = 0; i <= 15; i++) {
                printf("%4i: %08x\n", i, initial_regs[i]);
            }

            printf("\nFinal Register Listing: \n");
            printf("      interp   jit\n");
            for (int i = 0; i <= 15; i++) {
                printf("%4i: %08x %08x %s\n", i, interp.Reg[i], jit.Regs()[i], interp.Reg[i] != jit.Regs()[i] ? "*" : "");
            }
            printf("CPSR: %08x %08x %s\n", interp.Cpsr, jit.Cpsr(), interp.Cpsr != jit.Cpsr() ? "*" : "");

#ifdef _MSC_VER
            __debugbreak();
#endif
            FAIL();
        }

        if (run_number % 10 == 0) printf("%zu\r", run_number);
    }
}

TEST_CASE("Fuzz ARM data processing instructions", "[JitX64]") {
    const std::array<InstructionGenerator, 16> imm_instructions = {
            {
                    InstructionGenerator("cccc0010101Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0010100Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0010000Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0011110Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc00110111nnnn0000rrrrvvvvvvvv"),
                    InstructionGenerator("cccc00110101nnnn0000rrrrvvvvvvvv"),
                    InstructionGenerator("cccc0010001Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0011101S0000ddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0011111S0000ddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0011100Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0010011Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0010111Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0010110Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc0010010Snnnnddddrrrrvvvvvvvv"),
                    InstructionGenerator("cccc00110011nnnn0000rrrrvvvvvvvv"),
                    InstructionGenerator("cccc00110001nnnn0000rrrrvvvvvvvv"),
            }
    };

    const std::array<InstructionGenerator, 16> reg_instructions = {
            {
                    InstructionGenerator("cccc0000101Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0000100Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0000000Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0001110Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc00010111nnnn0000vvvvvrr0mmmm"),
                    InstructionGenerator("cccc00010101nnnn0000vvvvvrr0mmmm"),
                    InstructionGenerator("cccc0000001Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0001101S0000ddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0001111S0000ddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0001100Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0000011Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0000111Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0000110Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc0000010Snnnnddddvvvvvrr0mmmm"),
                    InstructionGenerator("cccc00010011nnnn0000vvvvvrr0mmmm"),
                    InstructionGenerator("cccc00010001nnnn0000vvvvvrr0mmmm"),
            }
    };

    const std::array<InstructionGenerator, 16> rsr_instructions = {
            {
                    InstructionGenerator("cccc0000101Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0000100Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0000000Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0001110Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc00010111nnnn0000ssss0rr1mmmm"),
                    InstructionGenerator("cccc00010101nnnn0000ssss0rr1mmmm"),
                    InstructionGenerator("cccc0000001Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0001101S0000ddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0001111S0000ddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0001100Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0000011Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0000111Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0000110Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc0000010Snnnnddddssss0rr1mmmm"),
                    InstructionGenerator("cccc00010011nnnn0000ssss0rr1mmmm"),
                    InstructionGenerator("cccc00010001nnnn0000ssss0rr1mmmm"),
            }
    };

    auto instruction_select = [&](bool Rd_can_be_r15) -> auto {
        return [&, Rd_can_be_r15]() -> u32 {
            size_t instruction_set = RandInt<size_t>(0, 2);

            u32 cond = 0xE;
            // Have a one-in-twenty-five chance of actually having a cond.
            if (RandInt(1, 25) == 1) {
                cond = RandInt<u32>(0x0, 0xD);
            }

            u32 S = RandInt<u32>(0, 1);

            switch (instruction_set) {
                case 0: {
                    InstructionGenerator instruction = imm_instructions[RandInt<size_t>(0, imm_instructions.size() - 1)];
                    u32 Rd = RandInt<u32>(0, Rd_can_be_r15 ? 15 : 14);
                    if (Rd == 15) S = false;
                    u32 Rn = RandInt<u32>(0, 15);
                    u32 shifter_operand = RandInt<u32>(0, 0xFFF);
                    u32 assemble_randoms = (shifter_operand << 0) | (Rd << 12) | (Rn << 16) | (S << 20) | (cond << 28);
                    return instruction.Bits() | (assemble_randoms & ~instruction.Mask());
                }
                case 1: {
                    InstructionGenerator instruction = reg_instructions[RandInt<size_t>(0, reg_instructions.size() - 1)];
                    u32 Rd = RandInt<u32>(0, Rd_can_be_r15 ? 15 : 14);
                    if (Rd == 15) S = false;
                    u32 Rn = RandInt<u32>(0, 15);
                    u32 shifter_operand = RandInt<u32>(0, 0xFFF);
                    u32 assemble_randoms =
                            (shifter_operand << 0) | (Rd << 12) | (Rn << 16) | (S << 20) | (cond << 28);
                    return instruction.Bits() | (assemble_randoms & ~instruction.Mask());
                }
                case 2: {
                    InstructionGenerator instruction = rsr_instructions[RandInt<size_t>(0, rsr_instructions.size() - 1)];
                    u32 Rd = RandInt<u32>(0, 14); // Rd can never be 15.
                    u32 Rn = RandInt<u32>(0, 14);
                    u32 Rs = RandInt<u32>(0, 14);
                    int rotate = RandInt<int>(0, 3);
                    u32 Rm = RandInt<u32>(0, 14);
                    u32 assemble_randoms =
                            (Rm << 0) | (rotate << 5) | (Rs << 8) | (Rd << 12) | (Rn << 16) | (S << 20) | (cond << 28);
                    return instruction.Bits() | (assemble_randoms & ~instruction.Mask());
                }
            }
            return 0;
        };
    };

    SECTION("short blocks") {
        FuzzJitArm(5, 6, 10000, instruction_select(/*Rd_can_be_r15=*/false));
    }

    SECTION("long blocks") {
        FuzzJitArm(1024, 1025, 200, instruction_select(/*Rd_can_be_r15=*/false));
    }

    SECTION("R15") {
        FuzzJitArm(1, 1, 10000, instruction_select(/*Rd_can_be_r15=*/true));
    }
}

TEST_CASE("Fuzz ARM reversal instructions", "[JitX64]") {
    const auto is_valid = [](u32 instr) -> bool {
        // R15 is UNPREDICTABLE
        return Dynarmic::Common::Bits<0, 3>(instr) != 0b1111 && Dynarmic::Common::Bits<12, 15>(instr) != 0b1111;
    };

    const std::array<InstructionGenerator, 3> rev_instructions = {
        {
            InstructionGenerator("0000011010111111dddd11110011mmmm", is_valid),
            InstructionGenerator("0000011010111111dddd11111011mmmm", is_valid),
            InstructionGenerator("0000011011111111dddd11111011mmmm", is_valid),
        }
    };

    SECTION("REV tests") {
        FuzzJitArm(1, 1, 10000, [&rev_instructions]() -> u32 {
            u32 cond = 0xE;
            // Have a one-in-twenty-five chance of actually having a cond.
            if (RandInt(1, 25) == 1) {
                cond = RandInt<u32>(0x0, 0xD);
            }
            return rev_instructions[0].Generate() | (cond << 28);
        });
    }

    SECTION("REV16 tests") {
        FuzzJitArm(1, 1, 10000, [&rev_instructions]() -> u32 {
            u32 cond = 0xE;
            // Have a one-in-twenty-five chance of actually having a cond.
            if (RandInt(1, 25) == 1) {
                cond = RandInt<u32>(0x0, 0xD);
            }
            return rev_instructions[1].Generate() | (cond << 28);
        });
    }

    SECTION("REVSH tests") {
        FuzzJitArm(1, 1, 10000, [&rev_instructions]() -> u32 {
            u32 cond = 0xE;
            // Have a one-in-twenty-five chance of actually having a cond.
            if (RandInt(1, 25) == 1) {
                cond = RandInt<u32>(0x0, 0xD);
            }
            return rev_instructions[2].Generate() | (cond << 28);
        });
    }
}

TEST_CASE("Fuzz ARM Load/Store instructions", "[JitX64]") {
    auto forbid_r15 = [](u32 inst) -> bool {
        return Dynarmic::Common::Bits<12, 15>(inst) != 0b1111;
    };

    auto forbid_r14_and_r15 = [](u32 inst) -> bool {
        return Dynarmic::Common::Bits<13, 15>(inst) != 0b111;
    };

    const std::array<InstructionGenerator, 4> doubleword_instructions = {
        {
            // Load
            InstructionGenerator("0000000pu1w0nnnnddd0vvvv1101vvvv", forbid_r14_and_r15),
            InstructionGenerator("0000000pu0w0nnnnddd000001101mmmm", forbid_r14_and_r15),

            // Store
            InstructionGenerator("0000000pu1w0nnnnddd0vvvv1111vvvv", forbid_r14_and_r15),
            InstructionGenerator("0000000pu0w0nnnnddd000001111mmmm", forbid_r14_and_r15),
        }
    };

    const std::array<InstructionGenerator, 8> word_instructions = {
        {
            // Load
            InstructionGenerator("0000010pu0w1nnnnddddvvvvvvvvvvvv", forbid_r15),
            InstructionGenerator("0000011pu0w1nnnnddddvvvvvrr0mmmm", forbid_r15),
            InstructionGenerator("00000100u011nnnnttttmmmmmmmmmmmm", forbid_r15),
            InstructionGenerator("00000110u011nnnnttttvvvvvrr0mmmm", forbid_r15),

            // Store
            InstructionGenerator("0000010pu0w0nnnnddddvvvvvvvvvvvv", forbid_r15),
            InstructionGenerator("0000011pu0w0nnnnddddvvvvvrr0mmmm", forbid_r15),
            InstructionGenerator("00000100u010nnnnttttvvvvvvvvvvvv", forbid_r15),
            InstructionGenerator("00000110u010nnnnttttvvvvvrr0mmmm", forbid_r15),
        }
    };

    const std::array<InstructionGenerator, 6> halfword_instructions = {
        {
            // Load
            InstructionGenerator("0000000pu1w1nnnnddddvvvv1011vvvv", forbid_r15),
            InstructionGenerator("0000000pu0w1nnnndddd00001011mmmm", forbid_r15),
            // InstructionGenerator("----0000-111------------1011----"), // LDRHT (A1) Not available in ARMv6K
            // InstructionGenerator("----0000-011--------00001011----"), // LDRHT (A2) Not available in ARMv6K
            InstructionGenerator("0000000pu1w1nnnnddddvvvv1111vvvv", forbid_r15),
            InstructionGenerator("0000000pu0w1nnnndddd00001111mmmm", forbid_r15),
            // InstructionGenerator("----0000-111------------1111----"), // LDRSHT (A1) Not available in ARMv6K
            // InstructionGenerator("----0000-011--------00001111----"), // LDRSHT (A2) Not available in ARMv6K


            // Store
            InstructionGenerator("0000000pu1w0nnnnddddvvvv1011vvvv", forbid_r15),
            InstructionGenerator("0000000pu0w0nnnndddd00001011mmmm", forbid_r15),
            // InstructionGenerator("----0000-110------------1011----"), // STRHT (A1) Not available in ARMv6K
            // InstructionGenerator("----0000-010--------00001011----"), // STRHT (A2) Not available in ARMv6K
        }
    };

    const std::array<InstructionGenerator, 10> byte_instructions = {
        {
            // Load
            InstructionGenerator("0000010pu1w1nnnnddddvvvvvvvvvvvv", forbid_r15),
            InstructionGenerator("0000011pu1w1nnnnddddvvvvvrr0mmmm", forbid_r15),
            InstructionGenerator("00000100u111nnnnttttvvvvvvvvvvvv", forbid_r15),
            InstructionGenerator("00000110u111nnnnttttvvvvvrr0mmmm", forbid_r15),
            InstructionGenerator("0000000pu1w1nnnnddddvvvv1101vvvv", forbid_r15),
            InstructionGenerator("0000000pu0w1nnnndddd00001101mmmm", forbid_r15),
            // InstructionGenerator("----0000-111------------1101----"), // LDRSBT (A1) Not available in ARMv6K
            // InstructionGenerator("----0000-011--------00001101----"), // LDRSBT (A2) Not available in ARMv6K


            // Store
            InstructionGenerator("0000010pu1w0nnnnddddvvvvvvvvvvvv", forbid_r15),
            InstructionGenerator("0000011pu1w0nnnnddddvvvvvrr0mmmm", forbid_r15),
            InstructionGenerator("00000100u110nnnnttttvvvvvvvvvvvv", forbid_r15),
            InstructionGenerator("00000110u110nnnnttttvvvvvrr0mmmm", forbid_r15),
        }
    };

    SECTION("Doubleword tests") {
        FuzzJitArm(1, 1, 10000, [&doubleword_instructions]() -> u32 {
            u32 cond = 0xE;
            // Have a one-in-twenty-five chance of actually having a cond.
            if (RandInt(1, 25) == 1) {
                cond = RandInt<u32>(0x0, 0xD);
            }

            return doubleword_instructions[RandInt<size_t>(0, doubleword_instructions.size() - 1)].Generate() | (cond << 28);
        });
    }

    SECTION("Word tests") {
        FuzzJitArm(1, 1, 10000, [&word_instructions]() -> u32 {
            u32 cond = 0xE;
            // Have a one-in-twenty-five chance of actually having a cond.
            if (RandInt(1, 25) == 1) {
                cond = RandInt<u32>(0x0, 0xD);
            }
            return word_instructions[RandInt<size_t>(0, word_instructions.size() - 1)].Generate() | (cond << 28);
        });
    }

    SECTION("Halfword tests") {
        FuzzJitArm(1, 1, 10000, [&halfword_instructions]() -> u32 {
            u32 cond = 0xE;
            // Have a one-in-twenty-five chance of actually having a cond.
            if (RandInt(1, 25) == 1) {
                cond = RandInt<u32>(0x0, 0xD);
            }
            return halfword_instructions[RandInt<size_t>(0, halfword_instructions.size() - 1)].Generate() | (cond << 28);
        });
    }

    SECTION("Byte tests") {
        FuzzJitArm(1, 1, 10000, [&byte_instructions]() -> u32 {
            u32 cond = 0xE;
            // Have a one-in-twenty-five chance of actually having a cond.
            if (RandInt(1, 25) == 1) {
                cond = RandInt<u32>(0x0, 0xD);
            }
            return byte_instructions[RandInt<size_t>(0, byte_instructions.size() - 1)].Generate() | (cond << 28);
        });
    }

    SECTION("Mixed tests") {
        FuzzJitArm(10, 10, 10000, [&]() -> u32 {
            size_t selection = RandInt<size_t>(0, 3);

            u32 cond = 0xE;
            // Have a one-in-twenty-five chance of actually having a cond.
            if (RandInt(1, 25) == 1) {
                cond = RandInt<u32>(0x0, 0xD);
            }

            switch (selection) {
            case 0:
                return doubleword_instructions[RandInt<size_t>(0, doubleword_instructions.size() - 1)].Generate() | (cond << 28);
            case 1:
                return word_instructions[RandInt<size_t>(0, word_instructions.size() - 1)].Generate() | (cond << 28);
            case 2:
                return halfword_instructions[RandInt<size_t>(0, halfword_instructions.size() - 1)].Generate() | (cond << 28);
            case 3:
                return byte_instructions[RandInt<size_t>(0, byte_instructions.size() - 1)].Generate() | (cond << 28);
            }

            return 0;
        });
    }

    SECTION("Write to PC") {
        // TODO
        FAIL();
    }
}