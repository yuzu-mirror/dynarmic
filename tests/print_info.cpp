/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <cstring>
#include <cstdlib>

#include <dynarmic/A32/disassembler.h>
#include <fmt/format.h>

#include "common/common_types.h"
#include "common/llvm_disassemble.h"
#include "frontend/A32/decoder/arm.h"
#include "frontend/A32/location_descriptor.h"
#include "frontend/A32/translate/impl/translate_arm.h"
#include "frontend/A32/translate/translate.h"
#include "frontend/A64/decoder/a64.h"
#include "frontend/A64/location_descriptor.h"
#include "frontend/A64/translate/impl/impl.h"
#include "frontend/A64/translate/translate.h"
#include "frontend/ir/basic_block.h"

using namespace Dynarmic;

const char* GetNameOfA32Instruction(u32 instruction) {
    if (auto decoder = A32::DecodeArm<A32::ArmTranslatorVisitor>(instruction)) {
        return decoder->get().GetName();
    }
    return "<null>";
}

const char* GetNameOfA64Instruction(u32 instruction) {
    if (auto decoder = A64::Decode<A64::TranslatorVisitor>(instruction)) {
        return decoder->get().GetName();
    }
    return "<null>";
}

void PrintA32Instruction(u32 instruction) {
    fmt::print("{:08x} {}\n", instruction, A32::DisassembleArm(instruction));
    fmt::print("Name: {}\n", GetNameOfA32Instruction(instruction));

    const A32::LocationDescriptor location{0, {}, {}};
    IR::Block block{location};
    const bool should_continue = A32::TranslateSingleInstruction(block, location, instruction);
    fmt::print("should_continue: {}\n", should_continue);
    fmt::print("IR:\n");
    fmt::print("{}\n", IR::DumpBlock(block));
}

void PrintA64Instruction(u32 instruction) {
    fmt::print("{:08x} {}\n", instruction, Common::DisassembleAArch64(instruction));
    fmt::print("Name: {}\n", GetNameOfA64Instruction(instruction));

    const A64::LocationDescriptor location{0, {}};
    IR::Block block{location};
    const bool should_continue = A64::TranslateSingleInstruction(block, location, instruction);
    fmt::print("should_continue: {}\n", should_continue);
    fmt::print("IR:\n");
    fmt::print("{}\n", IR::DumpBlock(block));
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fmt::print("usage: {} <a32/a64> <instruction_in_hex>\n", argv[0]);
        return 1;
    }

    if (strlen(argv[2]) > 8) {
        fmt::print("hex string too long\n");
        return 1;
    }

    const u32 instruction = strtol(argv[2], nullptr, 16);

    if (strcmp(argv[1], "a32") == 0) {
        PrintA32Instruction(instruction);
    } else if (strcmp(argv[1], "a64") == 0) {
        PrintA64Instruction(instruction);
    } else {
        fmt::print("Invalid mode: {}\nValid values: a32, a64\n", argv[1]);
    }

    return 0;
}
