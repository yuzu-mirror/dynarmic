/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <cinttypes>
#include <cstring>

#include <catch.hpp>

#include "common/common_types.h"
#include "frontend/disassembler.h"
#include "interface/interface.h"
#include "rand_int.h"
#include "skyeye_interpreter/dyncom/arm_dyncom_interpreter.h"
#include "skyeye_interpreter/skyeye_common/armstate.h"

static std::array<u16, 3000> code_mem{};

static u32 MemoryRead32(u32 vaddr);
static void InterpreterFallback(u32 pc, Dynarmic::Jit* jit);
static Dynarmic::UserCallbacks GetUserCallbacks();

static u32 MemoryRead32(u32 vaddr) {
    if (vaddr < code_mem.size() * sizeof(u16)) {
        size_t index = vaddr / sizeof(u16);
        return code_mem[index] | (code_mem[index+1] << 16);
    }
    return vaddr;
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

    jit->Regs() = interp_state.Reg;
    jit->Cpsr() = interp_state.Cpsr;
}

static void Fail() {
    FAIL();
}

static Dynarmic::UserCallbacks GetUserCallbacks() {
    Dynarmic::UserCallbacks user_callbacks{};
    user_callbacks.MemoryRead32 = &MemoryRead32;
    user_callbacks.InterpreterFallback = &InterpreterFallback;
    user_callbacks.CallSVC = (bool (*)(u32)) &Fail;
    user_callbacks.IsReadOnlyMemory = (bool (*)(u32)) &Fail;
    user_callbacks.MemoryRead8 = (u8 (*)(u32)) &Fail;
    user_callbacks.MemoryRead16 = (u16 (*)(u32)) &Fail;
    user_callbacks.MemoryRead64 = (u64 (*)(u32)) &Fail;
    user_callbacks.MemoryWrite8 = (void (*)(u32, u8)) &Fail;
    user_callbacks.MemoryWrite16 = (void (*)(u32, u16)) &Fail;
    user_callbacks.MemoryWrite32 = (void (*)(u32, u32)) &Fail;
    user_callbacks.MemoryWrite64 = (void (*)(u32, u64)) &Fail;
    return user_callbacks;
}

static std::pair<u16, u16> FromBitString16(const char* str) {
    REQUIRE(strlen(str) == 16);

    u16 bits = 0;
    u16 mask = 0;
    for (int i = 0; i < 16; i++) {
        const u16 bit = 1 << (15 - i);
        switch (str[i]) {
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
    return { bits, mask };
}

static bool DoesBehaviorMatch(const ARMul_State& interp, const Dynarmic::Jit& jit) {
    const auto interp_regs = interp.Reg;
    const auto jit_regs = jit.Regs();

    return std::equal(interp_regs.begin(), interp_regs.end(), jit_regs.begin(), jit_regs.end())
           && interp.Cpsr == jit.Cpsr();
}


void FuzzJitThumb(const size_t instruction_count, const size_t instructions_to_execute_count, const size_t run_count, const std::function<u16()> instruction_generator) {
    // Prepare memory
    code_mem.fill(0xE7FE); // b +#0

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

        interp.Cpsr = 0x000001F0;
        interp.Reg = initial_regs;
        jit.Cpsr() = 0x000001F0;
        jit.Regs() = initial_regs;

        std::generate_n(code_mem.begin(), instruction_count, instruction_generator);

        // Run interpreter
        interp.NumInstrsToExecute = instructions_to_execute_count;
        InterpreterMainLoop(&interp);

        // Run jit
        jit.Run(instructions_to_execute_count);

        // Compare
        if (!DoesBehaviorMatch(interp, jit)) {
            printf("Failed at execution number %zu\n", run_number);

            printf("\nInstruction Listing: \n");
            for (size_t i = 0; i < instruction_count; i++) {
                printf("%s\n", Dynarmic::Arm::DisassembleThumb16(code_mem[i]).c_str());
            }

            printf("\nFinal Register Listing: \n");
            for (int i = 0; i <= 15; i++) {
                printf("%4i: %08x %08x %s\n", i, interp.Reg[i], jit.Regs()[i], interp.Reg[i] != jit.Regs()[i] ? "*" : "");
            }
            printf("CPSR: %08x %08x %s\n", interp.Cpsr, jit.Cpsr(), interp.Cpsr != jit.Cpsr() ? "*" : "");

#ifdef _MSC_VER
            DebugBreak();
#endif
            FAIL();
        }

        if (run_number % 10 == 0) printf("%zu\r", run_number);
    }
}

TEST_CASE("Fuzz Thumb instructions set 1", "[JitX64][Thumb]") {
    const std::array<std::pair<u16, u16>, 16> instructions = {{
        FromBitString16("00000xxxxxxxxxxx"), // LSL <Rd>, <Rm>, #<imm5>
        FromBitString16("00001xxxxxxxxxxx"), // LSR <Rd>, <Rm>, #<imm5>
        FromBitString16("00010xxxxxxxxxxx"), // ASR <Rd>, <Rm>, #<imm5>
        FromBitString16("000110oxxxxxxxxx"), // ADD/SUB_reg
        FromBitString16("000111oxxxxxxxxx"), // ADD/SUB_imm
        FromBitString16("001ooxxxxxxxxxxx"), // ADD/SUB/CMP/MOV_imm
        FromBitString16("010000ooooxxxxxx"), // Data Processing
        FromBitString16("010001000hxxxxxx"), // ADD (high registers)
        FromBitString16("010001010hxxxxxx"), // CMP (high registers)
        FromBitString16("01000101h0xxxxxx"), // CMP (high registers)
        FromBitString16("010001100hxxxxxx"), // MOV (high registers)
        FromBitString16("10110000oxxxxxxx"), // Adjust stack pointer
        FromBitString16("10110010ooxxxxxx"), // SXT/UXT
        FromBitString16("1011101000xxxxxx"), // REV
        FromBitString16("1011101001xxxxxx"), // REV16
        FromBitString16("1011101011xxxxxx"), // REVSH
        //FromBitString16("01001xxxxxxxxxxx"), // LDR Rd, [PC, #]
        //FromBitString16("0101oooxxxxxxxxx"), // LDR/STR Rd, [Rn, Rm]
        //FromBitString16("011xxxxxxxxxxxxx"), // LDR(B)/STR(B) Rd, [Rn, #]
        //FromBitString16("1000xxxxxxxxxxxx"), // LDRH/STRH Rd, [Rn, #offset]
        //FromBitString16("1001xxxxxxxxxxxx"), // LDR/STR Rd, [SP, #]
        //FromBitString16("1011x100xxxxxxxx"), // PUSH/POP (R = 0)
        //FromBitString16("1100xxxxxxxxxxxx"), // STMIA/LDMIA
        //FromBitString16("101101100101x000"), // SETEND
    }};

    auto instruction_select = [&]() -> u16 {
        size_t inst_index = RandInt<size_t>(0, instructions.size() - 1);

        if (inst_index == 22) {
            u16 L = RandInt<u16>(0, 1);
            u16 Rn = RandInt<u16>(0, 7);
            u16 reg_list = RandInt<u16>(1, 0xFF);
            if (!L && (reg_list & (1 << Rn))) {
                reg_list &= ~((1 << Rn) - 1);
                if (reg_list == 0) reg_list = 0x80;
            }
            u16 random = (L << 11) | (Rn << 8) | reg_list;
            return instructions[inst_index].first | (random &~instructions[inst_index].second);
        } else if (inst_index == 21) {
            u16 L = RandInt<u16>(0, 1);
            u16 reg_list = RandInt<u16>(1, 0xFF);
            u16 random = (L << 11) | reg_list;
            return instructions[inst_index].first | (random &~instructions[inst_index].second);
        } else {
            u16 random = RandInt<u16>(0, 0xFFFF);
            return instructions[inst_index].first | (random &~instructions[inst_index].second);
        }
    };

    SECTION("short blocks") {
        FuzzJitThumb(5, 6, 1000, instruction_select);
    }

    SECTION("long blocks") {
        FuzzJitThumb(1024, 1025, 25, instruction_select);
    }
}
