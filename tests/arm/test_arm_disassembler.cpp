/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <catch.hpp>

#include "frontend/disassembler.h"

TEST_CASE( "Disassemble branch instructions", "[arm][disassembler]" ) {
    REQUIRE(Dynarmic::Arm::DisassembleArm(0xEAFFFFFE) == "b +#0");
    REQUIRE(Dynarmic::Arm::DisassembleArm(0xEB000008) == "bl +#40");
    REQUIRE(Dynarmic::Arm::DisassembleArm(0xFBFFFFFE) == "blx +#2");
    REQUIRE(Dynarmic::Arm::DisassembleArm(0xFAFFFFFF) == "blx +#4");
    REQUIRE(Dynarmic::Arm::DisassembleArm(0xFBE1E7FE) == "blx -#7888894");
    REQUIRE(Dynarmic::Arm::DisassembleArm(0xE12FFF3D) == "blx sp");
    REQUIRE(Dynarmic::Arm::DisassembleArm(0x312FFF13) == "bxcc r3");
    REQUIRE(Dynarmic::Arm::DisassembleArm(0x012FFF29) == "bxjeq r9");
}

TEST_CASE( "Disassemble data processing instructions", "[arm][disassembler]" ) {
    REQUIRE(Dynarmic::Arm::DisassembleArm(0xE2853004) == "add r3, r5, #4");
}
