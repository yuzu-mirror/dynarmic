/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <map>
#include <unordered_map>

#include "backend_x64/emit_x64.h"
#include "common/x64/abi.h"
#include "common/x64/emitter.h"
#include "frontend/arm_types.h"

// TODO: More optimal use of immediates.
// TODO: Have ARM flags in host flags and not have them use up GPR registers unless necessary.
// TODO: Actually implement that proper instruction selector you've always wanted to sweetheart.

using namespace Gen;

namespace Dynarmic {
namespace BackendX64 {

static OpArg MJitStateReg(Arm::Reg reg) {
    return MDisp(R15, offsetof(JitState, Reg) + sizeof(u32) * static_cast<size_t>(reg));
}

static OpArg MJitStateCpsr() {
    return MDisp(R15, offsetof(JitState, Cpsr));
}

static IR::Inst* FindUseWithOpcode(IR::Inst* inst, IR::Opcode opcode) {
    // Gets first found use.
    auto uses = inst->GetUses();
    auto iter = std::find_if(uses.begin(), uses.end(), [opcode](const auto& use){ return use->GetOpcode() == opcode; });
    ASSERT(std::count_if(uses.begin(), uses.end(), [opcode](const auto& use){ return use->GetOpcode() == opcode; }) <= 1);
    return iter == uses.end() ? nullptr : reinterpret_cast<IR::Inst*>(iter->get());
}

CodePtr EmitX64::Emit(const Arm::LocationDescriptor descriptor, const Dynarmic::IR::Block& block) {
    inhibit_emission.clear();
    reg_alloc.Reset();

    code->INT3();
    CodePtr code_ptr = code->GetCodePtr();
    basic_blocks[descriptor] = code_ptr;

    EmitCondPrelude(block.cond, block.cond_failed, block.location);

    for (const auto& value : block.instructions) {
        if (inhibit_emission.count(value.get()) != 0)
            continue;

        // Call the relevant Emit* member function.
        switch (value->GetOpcode()) {

#define OPCODE(name, type, ...)                   \
            case IR::Opcode::name:                \
                EmitX64::Emit##name(value.get()); \
                break;
#include "frontend/ir/opcodes.inc"
#undef OPCODE

            default:
                ASSERT_MSG(false, "Invalid opcode %zu", static_cast<size_t>(value->GetOpcode()));
                break;
        }

        reg_alloc.EndOfAllocScope();
    }

    EmitAddCycles(block.cycle_count);
    EmitTerminal(block.terminal, block.location);

    reg_alloc.AssertNoMoreUses();

    return code_ptr;
}

void EmitX64::EmitImmU1(IR::Value* value_) {
    auto value = reinterpret_cast<IR::ImmU1*>(value_);

    X64Reg result = reg_alloc.DefRegister(value);

    code->MOV(32, R(result), Imm32(value->value));
}

void EmitX64::EmitImmU8(IR::Value* value_) {
    auto value = reinterpret_cast<IR::ImmU8*>(value_);

    X64Reg result = reg_alloc.DefRegister(value);

    code->MOV(32, R(result), Imm32(value->value));
}

void EmitX64::EmitImmU32(IR::Value* value_) {
    auto value = reinterpret_cast<IR::ImmU32*>(value_);

    X64Reg result = reg_alloc.DefRegister(value);

    code->MOV(32, R(result), Imm32(value->value));
}

void EmitX64::EmitImmRegRef(IR::Value*) {
    return; // No need to do anything.
}

void EmitX64::EmitGetRegister(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);
    auto regref = reinterpret_cast<IR::ImmRegRef*>(value->GetArg(0).get());

    X64Reg result = reg_alloc.DefRegister(value);

    code->MOV(32, R(result), MJitStateReg(regref->value));
}

void EmitX64::EmitSetRegister(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);
    auto regref = reinterpret_cast<IR::ImmRegRef*>(value->GetArg(0).get());

    X64Reg to_store = reg_alloc.UseRegister(value->GetArg(1).get());

    code->MOV(32, MJitStateReg(regref->value), R(to_store));
}

void EmitX64::EmitGetNFlag(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg result = reg_alloc.DefRegister(value);

    // TODO: Flag optimization

    code->MOV(32, R(result), MJitStateCpsr());
    code->SHR(32, R(result), Imm8(31));
}

void EmitX64::EmitSetNFlag(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg to_store = reg_alloc.UseRegister(value->GetArg(0).get());

    // TODO: Flag optimization

    code->SHL(32, R(to_store), Imm8(31));
    code->AND(32, MJitStateCpsr(), Imm32(~static_cast<u32>(1 << 31)));
    code->OR(32, MJitStateCpsr(), R(to_store));
}

void EmitX64::EmitGetZFlag(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg result = reg_alloc.DefRegister(value);

    // TODO: Flag optimization

    code->MOV(32, R(result), MJitStateCpsr());
    code->SHR(32, R(result), Imm8(30));
    code->AND(32, R(result), Imm32(1));
}

void EmitX64::EmitSetZFlag(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg to_store = reg_alloc.UseRegister(value->GetArg(0).get());

    // TODO: Flag optimization

    code->SHL(32, R(to_store), Imm8(30));
    code->AND(32, MJitStateCpsr(), Imm32(~static_cast<u32>(1 << 30)));
    code->OR(32, MJitStateCpsr(), R(to_store));
}

void EmitX64::EmitGetCFlag(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg result = reg_alloc.DefRegister(value);

    // TODO: Flag optimization

    code->MOV(32, R(result), MJitStateCpsr());
    code->SHR(32, R(result), Imm8(29));
    code->AND(32, R(result), Imm32(1));
}

void EmitX64::EmitSetCFlag(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg to_store = reg_alloc.UseRegister(value->GetArg(0).get());

    // TODO: Flag optimization

    code->SHL(32, R(to_store), Imm8(29));
    code->AND(32, MJitStateCpsr(), Imm32(~static_cast<u32>(1 << 29)));
    code->OR(32, MJitStateCpsr(), R(to_store));
}

void EmitX64::EmitGetVFlag(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg result = reg_alloc.DefRegister(value);

    // TODO: Flag optimization

    code->MOV(32, R(result), MJitStateCpsr());
    code->SHR(32, R(result), Imm8(28));
    code->AND(32, R(result), Imm32(1));
}

void EmitX64::EmitSetVFlag(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg to_store = reg_alloc.UseRegister(value->GetArg(0).get());

    // TODO: Flag optimization

    code->SHL(32, R(to_store), Imm8(28));
    code->AND(32, MJitStateCpsr(), Imm32(~static_cast<u32>(1 << 28)));
    code->OR(32, MJitStateCpsr(), R(to_store));
}

void EmitX64::EmitBXWritePC(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg new_pc = reg_alloc.UseRegister(value->GetArg(0).get());
    X64Reg tmp1 = reg_alloc.ScratchRegister();
    X64Reg tmp2 = reg_alloc.ScratchRegister();

    // Pseudocode:
    // if (new_pc & 1) {
    //    new_pc &= 0xFFFFFFFE;
    //    cpsr.T = true;
    // } else {
    //    new_pc &= 0xFFFFFFFC;
    //    cpsr.T = false;
    // }

    code->MOV(32, R(tmp1), MJitStateCpsr());
    code->MOV(32, R(tmp2), R(tmp1));
    code->AND(32, R(tmp2), Imm32(~(1 << 5)));      // CPSR.T = 0
    code->OR(32, R(tmp1), Imm32(1 << 5));          // CPSR.T = 1
    code->TEST(8, R(new_pc), Imm8(1));
    code->CMOVcc(32, tmp1, R(tmp2), CC_E);         // CPSR.T = pc & 1
    code->MOV(32, MJitStateCpsr(), R(tmp1));
    code->LEA(32, tmp2, MComplex(new_pc, new_pc, 1, 0));
    code->OR(32, R(tmp2), Imm32(0xFFFFFFFC));      // tmp2 = pc & 1 ? 0xFFFFFFFE : 0xFFFFFFFC
    code->AND(32, R(new_pc), R(tmp2));
    code->MOV(32, MJitStateReg(Arm::Reg::PC), R(new_pc));
}

void EmitX64::EmitCallSupervisor(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    auto imm32 = value->GetArg(0).get();
    reg_alloc.HostCall(nullptr, imm32);

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.CallSVC));
}

void EmitX64::EmitGetCarryFromOp(IR::Value*) {
    ASSERT_MSG(0, "should never happen");
}

void EmitX64::EmitGetOverflowFromOp(IR::Value*) {
    ASSERT_MSG(0, "should never happen");
}

void EmitX64::EmitLeastSignificantHalf(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    // TODO: Optimize

    reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
}

void EmitX64::EmitLeastSignificantByte(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    // TODO: Optimize

    reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
}

void EmitX64::EmitMostSignificantBit(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    // TODO: Flag optimization

    code->SHR(32, R(result), Imm8(31));
}

void EmitX64::EmitIsZero(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    // TODO: Flag optimization

    code->TEST(32, R(result), R(result));
    code->SETcc(CCFlags::CC_E, R(result));
    code->MOVZX(32, 8, result, R(result));
}

void EmitX64::EmitLogicalShiftLeft(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);
    auto carry_inst = FindUseWithOpcode(value, IR::Opcode::GetCarryFromOp);

    // TODO: Consider using BMI2 instructions like SHLX when arm-in-host flags is implemented.

    if (!carry_inst) {
        X64Reg shift = reg_alloc.UseRegister(value->GetArg(1).get(), {HostLoc::RCX});
        X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
        X64Reg zero = reg_alloc.ScratchRegister();

        // The 32-bit x64 SHL instruction masks the shift count by 0x1F before performing the shift.
        // ARM differs from the behaviour: It does not mask the count, so shifts above 31 result in zeros.

        code->SHL(32, R(result), R(shift));
        code->XOR(32, R(zero), R(zero));
        code->CMP(8, R(shift), Imm8(32));
        code->CMOVcc(32, result, R(zero), CC_NB);
    } else {
        inhibit_emission.insert(carry_inst);

        X64Reg shift = reg_alloc.UseRegister(value->GetArg(1).get(), {HostLoc::RCX});
        X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
        X64Reg carry = reg_alloc.UseDefRegister(value->GetArg(2).get(), carry_inst);

        reg_alloc.DecrementRemainingUses(value);

        // TODO: Optimize this.

        code->CMP(8, R(shift), Imm8(32));
        auto Rs_gt32 = code->J_CC(CC_A);
        auto Rs_eq32 = code->J_CC(CC_E);
        // if (Rs & 0xFF < 32) {
        code->BT(32, R(carry), Imm8(0)); // Set the carry flag for correct behaviour in the case when Rs & 0xFF == 0
        code->SHL(32, R(result), R(shift));
        code->SETcc(CC_C, R(carry));
        auto jmp_to_end_1 = code->J();
        // } else if (Rs & 0xFF > 32) {
        code->SetJumpTarget(Rs_gt32);
        code->XOR(32, R(result), R(result));
        code->XOR(32, R(carry), R(carry));
        auto jmp_to_end_2 = code->J();
        // } else if (Rs & 0xFF == 32) {
        code->SetJumpTarget(Rs_eq32);
        code->MOV(32, R(carry), R(result));
        code->AND(32, R(carry), Imm8(1));
        code->XOR(32, R(result), R(result));
        // }
        code->SetJumpTarget(jmp_to_end_1);
        code->SetJumpTarget(jmp_to_end_2);
    }
}

void EmitX64::EmitLogicalShiftRight(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);
    auto carry_inst = FindUseWithOpcode(value, IR::Opcode::GetCarryFromOp);

    if (!carry_inst) {
        X64Reg shift = reg_alloc.UseRegister(value->GetArg(1).get(), {HostLoc::RCX});
        X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
        X64Reg zero = reg_alloc.ScratchRegister();

        // The 32-bit x64 SHR instruction masks the shift count by 0x1F before performing the shift.
        // ARM differs from the behaviour: It does not mask the count, so shifts above 31 result in zeros.

        code->SHR(32, R(result), R(shift));
        code->XOR(32, R(zero), R(zero));
        code->CMP(8, R(shift), Imm8(32));
        code->CMOVcc(32, result, R(zero), CC_NB);
    } else {
        inhibit_emission.insert(carry_inst);

        X64Reg shift = reg_alloc.UseRegister(value->GetArg(1).get(), {HostLoc::RCX});
        X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
        X64Reg carry = reg_alloc.UseDefRegister(value->GetArg(2).get(), carry_inst);

        reg_alloc.DecrementRemainingUses(value);

        // TODO: Optimize this.

        code->CMP(8, R(shift), Imm8(32));
        auto Rs_gt32 = code->J_CC(CC_A);
        auto Rs_eq32 = code->J_CC(CC_E);
        // if (Rs & 0xFF == 0) goto end;
        code->TEST(8, R(shift), R(shift));
        auto Rs_zero = code->J_CC(CC_Z);
        // if (Rs & 0xFF < 32) {
        code->SHR(32, R(result), R(shift));
        code->SETcc(CC_C, R(carry));
        auto jmp_to_end_1 = code->J();
        // } else if (Rs & 0xFF > 32) {
        code->SetJumpTarget(Rs_gt32);
        code->MOV(32, R(result), Imm32(0));
        code->MOV(8, R(carry), Imm8(0));
        auto jmp_to_end_2 = code->J();
        // } else if (Rs & 0xFF == 32) {
        code->SetJumpTarget(Rs_eq32);
        code->BT(32, R(result), Imm8(31));
        code->SETcc(CC_C, R(carry));
        code->MOV(32, R(result), Imm32(0));
        // }
        code->SetJumpTarget(jmp_to_end_1);
        code->SetJumpTarget(jmp_to_end_2);
        code->SetJumpTarget(Rs_zero);
    }
}

void EmitX64::EmitArithmeticShiftRight(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);
    auto carry_inst = FindUseWithOpcode(value, IR::Opcode::GetCarryFromOp);

    if (!carry_inst) {
        X64Reg shift = reg_alloc.UseRegister(value->GetArg(1).get(), {HostLoc::RCX});
        X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
        X64Reg const31 = reg_alloc.ScratchRegister();

        // The 32-bit x64 SAR instruction masks the shift count by 0x1F before performing the shift.
        // ARM differs from the behaviour: It does not mask the count.

        // We note that all shift values above 31 have the same behaviour as 31 does, so we saturate `shift` to 31.
        code->MOV(32, R(const31), Imm32(31));
        code->MOVZX(32, 8, shift, R(shift));
        code->CMP(32, R(shift), Imm32(31));
        code->CMOVcc(32, shift, R(const31), CC_L);
        code->SAR(32, R(result), R(shift));
    } else {
        inhibit_emission.insert(carry_inst);

        X64Reg shift = reg_alloc.UseRegister(value->GetArg(1).get(), {HostLoc::RCX});
        X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
        X64Reg carry = reg_alloc.UseDefRegister(value->GetArg(2).get(), carry_inst);

        reg_alloc.DecrementRemainingUses(value);

        // TODO: Optimize this.

        code->CMP(8, R(shift), Imm8(31));
        auto Rs_gt31 = code->J_CC(CC_A);
        // if (Rs & 0xFF == 0) goto end;
        code->TEST(8, R(shift), R(shift));
        auto Rs_zero = code->J_CC(CC_Z);
        // if (Rs & 0xFF <= 31) {
        code->SAR(32, R(result), R(CL));
        code->SETcc(CC_C, R(carry));
        auto jmp_to_end = code->J();
        // } else if (Rs & 0xFF > 31) {
        code->SetJumpTarget(Rs_gt31);
        code->SAR(32, R(result), Imm8(31)); // Verified.
        code->BT(32, R(result), Imm8(31));
        code->SETcc(CC_C, R(carry));
        // }
        code->SetJumpTarget(jmp_to_end);
        code->SetJumpTarget(Rs_zero);
    }
}

void EmitX64::EmitRotateRight(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);
    auto carry_inst = FindUseWithOpcode(value, IR::Opcode::GetCarryFromOp);

    if (!carry_inst) {
        X64Reg shift = reg_alloc.UseRegister(value->GetArg(1).get(), {HostLoc::RCX});
        X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

        // x64 ROR instruction does (shift & 0x1F) for us.
        code->ROR(32, R(result), R(shift));
    } else {
        inhibit_emission.insert(carry_inst);

        X64Reg shift = reg_alloc.UseRegister(value->GetArg(1).get(), {HostLoc::RCX});
        X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
        X64Reg carry = reg_alloc.UseDefRegister(value->GetArg(2).get(), carry_inst);

        reg_alloc.DecrementRemainingUses(value);

        // TODO: Optimize

        // if (Rs & 0xFF == 0) goto end;
        code->TEST(8, R(shift), R(shift));
        auto Rs_zero = code->J_CC(CC_Z);

        code->AND(32, R(shift), Imm8(0x1F));
        auto zero_1F = code->J_CC(CC_Z);
        // if (Rs & 0x1F != 0) {
        code->ROR(32, R(result), R(shift));
        code->SETcc(CC_C, R(carry));
        auto jmp_to_end = code->J();
        // } else {
        code->SetJumpTarget(zero_1F);
        code->BT(32, R(result), Imm8(31));
        code->SETcc(CC_C, R(carry));
        // }
        code->SetJumpTarget(jmp_to_end);
        code->SetJumpTarget(Rs_zero);
    }
}

void EmitX64::EmitAddWithCarry(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);
    auto carry_inst = FindUseWithOpcode(value, IR::Opcode::GetCarryFromOp);
    auto overflow_inst = FindUseWithOpcode(value, IR::Opcode::GetOverflowFromOp);

    X64Reg addend = reg_alloc.UseRegister(value->GetArg(1).get());
    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
    X64Reg carry = carry_inst
                   ? reg_alloc.UseDefRegister(value->GetArg(2).get(), carry_inst)
                   : reg_alloc.UseRegister(value->GetArg(2).get());
    X64Reg overflow = overflow_inst
                      ? reg_alloc.DefRegister(overflow_inst)
                      : X64Reg::INVALID_REG;

    // TODO: Consider using LEA.

    code->BT(32, R(carry), Imm8(0)); // Sets x64 CF appropriately.
    code->ADC(32, R(result), R(addend));

    if (carry_inst) {
        inhibit_emission.insert(carry_inst);
        reg_alloc.DecrementRemainingUses(value);
        code->SETcc(Gen::CC_C, R(carry));
    }
    if (overflow_inst) {
        inhibit_emission.insert(overflow_inst);
        reg_alloc.DecrementRemainingUses(value);
        code->SETcc(Gen::CC_O, R(overflow));
    }
}

void EmitX64::EmitSubWithCarry(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);
    auto carry_inst = FindUseWithOpcode(value, IR::Opcode::GetCarryFromOp);
    auto overflow_inst = FindUseWithOpcode(value, IR::Opcode::GetOverflowFromOp);

    X64Reg addend = reg_alloc.UseRegister(value->GetArg(1).get());
    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);
    X64Reg carry = carry_inst
                   ? reg_alloc.UseDefRegister(value->GetArg(2).get(), carry_inst)
                   : reg_alloc.UseRegister(value->GetArg(2).get());
    X64Reg overflow = overflow_inst
                      ? reg_alloc.DefRegister(overflow_inst)
                      : X64Reg::INVALID_REG;

    // TODO: Consider using LEA.
    // TODO: Optimize case when result isn't used but flags are (use a CMP instruction instead).
    // Note that x64 CF is inverse of what the ARM carry flag is here.

    code->BT(32, R(carry), Imm8(0));
    code->CMC();
    code->SBB(32, R(result), R(addend));

    if (carry_inst) {
        inhibit_emission.insert(carry_inst);
        reg_alloc.DecrementRemainingUses(value);
        code->SETcc(Gen::CC_NC, R(carry));
    }
    if (overflow_inst) {
        inhibit_emission.insert(overflow_inst);
        reg_alloc.DecrementRemainingUses(value);
        code->SETcc(Gen::CC_O, R(overflow));
    }
}

void EmitX64::EmitAnd(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg andend = reg_alloc.UseRegister(value->GetArg(1).get());
    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->AND(32, R(result), R(andend));
}

void EmitX64::EmitEor(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg eorend = reg_alloc.UseRegister(value->GetArg(1).get());
    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->XOR(32, R(result), R(eorend));
}

void EmitX64::EmitOr(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg orend = reg_alloc.UseRegister(value->GetArg(1).get());
    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->OR(32, R(result), R(orend));
}

void EmitX64::EmitNot(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->NOT(32, R(result));
}

void EmitX64::EmitSignExtendHalfToWord(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    // TODO: Remove unnecessary mov that may occur here
    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->MOVSX(32, 16, result, R(result));
}

void EmitX64::EmitSignExtendByteToWord(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    // TODO: Remove unnecessary mov that may occur here
    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->MOVSX(32, 8, result, R(result));
}

void EmitX64::EmitZeroExtendHalfToWord(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    // TODO: Remove unnecessary mov that may occur here
    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->MOVZX(32, 16, result, R(result));
}

void EmitX64::EmitZeroExtendByteToWord(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    // TODO: Remove unnecessary mov that may occur here
    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->MOVZX(32, 8, result, R(result));
}

void EmitX64::EmitByteReverseWord(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->BSWAP(32, result);
}

void EmitX64::EmitByteReverseHalf(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    X64Reg result = reg_alloc.UseDefRegister(value->GetArg(0).get(), value);

    code->ROL(16, R(result), Imm8(8));
}


void EmitX64::EmitReadMemory8(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    reg_alloc.HostCall(value, value->GetArg(0).get());

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryRead8));
}

void EmitX64::EmitReadMemory16(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    reg_alloc.HostCall(value, value->GetArg(0).get());

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryRead16));
}

void EmitX64::EmitReadMemory32(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    reg_alloc.HostCall(value, value->GetArg(0).get());

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryRead32));
}

void EmitX64::EmitReadMemory64(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    reg_alloc.HostCall(value, value->GetArg(0).get());

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryRead64));
}

void EmitX64::EmitWriteMemory8(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    reg_alloc.HostCall(nullptr, value->GetArg(0).get(), value->GetArg(1).get());

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryWrite8));
}

void EmitX64::EmitWriteMemory16(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    reg_alloc.HostCall(nullptr, value->GetArg(0).get(), value->GetArg(1).get());

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryWrite16));
}

void EmitX64::EmitWriteMemory32(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    reg_alloc.HostCall(nullptr, value->GetArg(0).get(), value->GetArg(1).get());

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryWrite32));
}

void EmitX64::EmitWriteMemory64(IR::Value* value_) {
    auto value = reinterpret_cast<IR::Inst*>(value_);

    reg_alloc.HostCall(nullptr, value->GetArg(0).get(), value->GetArg(1).get());

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryWrite64));
}


void EmitX64::EmitAddCycles(size_t cycles) {
    ASSERT(cycles < std::numeric_limits<u32>::max());
    code->SUB(64, MDisp(R15, offsetof(JitState, cycles_remaining)), Imm32(static_cast<u32>(cycles)));
}

static CCFlags EmitCond(Gen::XEmitter* code, Arm::Cond cond) {
    // TODO: This code is a quick copy-paste-and-quickly-modify job from a previous JIT. Clean this up.

    auto NFlag = [code](X64Reg reg){
        code->MOV(32, R(reg), MJitStateCpsr());
        code->SHR(32, R(reg), Imm8(31));
        code->AND(32, R(reg), Imm32(1));
    };

    auto ZFlag = [code](X64Reg reg){
        code->MOV(32, R(reg), MJitStateCpsr());
        code->SHR(32, R(reg), Imm8(30));
        code->AND(32, R(reg), Imm32(1));
    };

    auto CFlag = [code](X64Reg reg){
        code->MOV(32, R(reg), MJitStateCpsr());
        code->SHR(32, R(reg), Imm8(29));
        code->AND(32, R(reg), Imm32(1));
    };

    auto VFlag = [code](X64Reg reg){
        code->MOV(32, R(reg), MJitStateCpsr());
        code->SHR(32, R(reg), Imm8(28));
        code->AND(32, R(reg), Imm32(1));
    };

    CCFlags cc;

    switch (cond) {
        case Arm::Cond::EQ: //z
            ZFlag(RAX);
            code->CMP(8, R(RAX), Imm8(0));
            cc = CC_NE;
            break;
        case Arm::Cond::NE: //!z
            ZFlag(RAX);
            code->CMP(8, R(RAX), Imm8(0));
            cc = CC_E;
            break;
        case Arm::Cond::CS: //c
            CFlag(RBX);
            code->CMP(8, R(RBX), Imm8(0));
            cc = CC_NE;
            break;
        case Arm::Cond::CC: //!c
            CFlag(RBX);
            code->CMP(8, R(RBX), Imm8(0));
            cc = CC_E;
            break;
        case Arm::Cond::MI: //n
            NFlag(RCX);
            code->CMP(8, R(RCX), Imm8(0));
            cc = CC_NE;
            break;
        case Arm::Cond::PL: //!n
            NFlag(RCX);
            code->CMP(8, R(RCX), Imm8(0));
            cc = CC_E;
            break;
        case Arm::Cond::VS: //v
            VFlag(RDX);
            code->CMP(8, R(RDX), Imm8(0));
            cc = CC_NE;
            break;
        case Arm::Cond::VC: //!v
            VFlag(RDX);
            code->CMP(8, R(RDX), Imm8(0));
            cc = CC_E;
            break;
        case Arm::Cond::HI: { //c & !z
            const X64Reg tmp = RSI;
            ZFlag(RAX);
            code->MOVZX(64, 8, tmp, R(RAX));
            CFlag(RBX);
            code->CMP(8, R(RBX), R(tmp));
            cc = CC_A;
            break;
        }
        case Arm::Cond::LS: { //!c | z
            const X64Reg tmp = RSI;
            ZFlag(RAX);
            code->MOVZX(64, 8, tmp, R(RAX));
            CFlag(RBX);
            code->CMP(8, R(RBX), R(tmp));
            cc = CC_BE;
            break;
        }
        case Arm::Cond::GE: { // n == v
            const X64Reg tmp = RSI;
            VFlag(RDX);
            code->MOVZX(64, 8, tmp, R(RDX));
            NFlag(RCX);
            code->CMP(8, R(RCX), R(tmp));
            cc = CC_E;
            break;
        }
        case Arm::Cond::LT: { // n != v
            const X64Reg tmp = RSI;
            VFlag(RDX);
            code->MOVZX(64, 8, tmp, R(RDX));
            NFlag(RCX);
            code->CMP(8, R(RCX), R(tmp));
            cc = CC_NE;
            break;
        }
        case Arm::Cond::GT: { // !z & (n == v)
            const X64Reg tmp = RSI;
            NFlag(RCX);
            code->MOVZX(64, 8, tmp, R(RCX));
            VFlag(RDX);
            code->XOR(8, R(tmp), R(RDX));
            ZFlag(RAX);
            code->OR(8, R(tmp), R(RAX));
            code->TEST(8, R(tmp), R(tmp));
            cc = CC_Z;
            break;
        }
        case Arm::Cond::LE: { // z | (n != v)
            X64Reg tmp = RSI;
            NFlag(RCX);
            code->MOVZX(64, 8, tmp, R(RCX));
            VFlag(RDX);
            code->XOR(8, R(tmp), R(RDX));
            ZFlag(RAX);
            code->OR(8, R(tmp), R(RAX));
            code->TEST(8, R(tmp), R(tmp));
            cc = CC_NZ;
            break;
        }
        default:
            ASSERT_MSG(0, "Unknown cond %zu", static_cast<size_t>(cond));
            break;
    }

    return cc;
}

void EmitX64::EmitCondPrelude(Arm::Cond cond,
                              boost::optional<Arm::LocationDescriptor> cond_failed,
                              Arm::LocationDescriptor initial_location) {
    if (cond == Arm::Cond::AL) {
        ASSERT(!cond_failed.is_initialized());
        return;
    }

    ASSERT(cond_failed.is_initialized());

    CCFlags cc = EmitCond(code, cond);

    // TODO: Improve, maybe.
    auto fixup = code->J_CC(cc);
    EmitAddCycles(1); // TODO: Proper cycle count
    EmitTerminalLinkBlock(IR::Term::LinkBlock{cond_failed.get()}, initial_location);
    code->SetJumpTarget(fixup);
}

void EmitX64::EmitTerminal(IR::Terminal terminal, Arm::LocationDescriptor initial_location) {
    switch (terminal.which()) {
    case 1:
        EmitTerminalInterpret(boost::get<IR::Term::Interpret>(terminal), initial_location);
        return;
    case 2:
        EmitTerminalReturnToDispatch(boost::get<IR::Term::ReturnToDispatch>(terminal), initial_location);
        return;
    case 3:
        EmitTerminalLinkBlock(boost::get<IR::Term::LinkBlock>(terminal), initial_location);
        return;
    case 4:
        EmitTerminalLinkBlockFast(boost::get<IR::Term::LinkBlockFast>(terminal), initial_location);
        return;
    case 5:
        EmitTerminalPopRSBHint(boost::get<IR::Term::PopRSBHint>(terminal), initial_location);
        return;
    case 6:
        EmitTerminalIf(boost::get<IR::Term::If>(terminal), initial_location);
        return;
    default:
        ASSERT_MSG(0, "Invalid Terminal. Bad programmer.");
        return;
    }
}

void EmitX64::EmitTerminalInterpret(IR::Term::Interpret terminal, Arm::LocationDescriptor initial_location) {
    ASSERT_MSG(terminal.next.TFlag == initial_location.TFlag, "Unimplemented");
    ASSERT_MSG(terminal.next.EFlag == initial_location.EFlag, "Unimplemented");

    code->MOV(64, R(ABI_PARAM1), Imm64(terminal.next.arm_pc));
    code->MOV(64, R(ABI_PARAM2), Imm64(reinterpret_cast<u64>(jit_interface)));
    code->MOV(32, MJitStateReg(Arm::Reg::PC), R(ABI_PARAM1));
    code->MOV(64, R(RSP), MDisp(R15, offsetof(JitState, save_host_RSP)));
    code->ABI_CallFunction(reinterpret_cast<void*>(cb.InterpreterFallback));
    routines->GenReturnFromRunCode(code); // TODO: Check cycles
}

void EmitX64::EmitTerminalReturnToDispatch(IR::Term::ReturnToDispatch, Arm::LocationDescriptor initial_location) {
    routines->GenReturnFromRunCode(code);
}

void EmitX64::EmitTerminalLinkBlock(IR::Term::LinkBlock terminal, Arm::LocationDescriptor initial_location) {
    ASSERT_MSG(terminal.next.EFlag == initial_location.EFlag, "Unimplemented");

    code->MOV(32, MJitStateReg(Arm::Reg::PC), Imm32(terminal.next.arm_pc));
    if (terminal.next.TFlag != initial_location.TFlag) {
        if (terminal.next.TFlag) {
            code->OR(32, MJitStateCpsr(), Imm32(1 << 5));
        } else {
            code->AND(32, MJitStateCpsr(), Imm32(~(1 << 5)));
        }
    }
    routines->GenReturnFromRunCode(code); // TODO: Check cycles, Properly do a link
}

void EmitX64::EmitTerminalLinkBlockFast(IR::Term::LinkBlockFast terminal, Arm::LocationDescriptor initial_location) {
    EmitTerminalLinkBlock(IR::Term::LinkBlock{terminal.next}, initial_location); // TODO: Implement
}

void EmitX64::EmitTerminalPopRSBHint(IR::Term::PopRSBHint, Arm::LocationDescriptor initial_location) {
    EmitTerminalReturnToDispatch({}, initial_location);  // TODO: Implement RSB
}

void EmitX64::EmitTerminalIf(IR::Term::If terminal, Arm::LocationDescriptor initial_location) {
    CCFlags cc = EmitCond(code, terminal.if_);
    auto fixup = code->J_CC(cc);
    EmitTerminal(terminal.else_, initial_location);
    code->SetJumpTarget(fixup);
    EmitTerminal(terminal.then_, initial_location);
}

void EmitX64::ClearCache() {
    basic_blocks.clear();
}

} // namespace BackendX64
} // namespace Dynarmic
