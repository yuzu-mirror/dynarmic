/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <map>
#include <unordered_map>
#include <common/bit_util.h>

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

static OpArg MJitStateExtReg(Arm::ExtReg reg) {
    if (reg >= Arm::ExtReg::S0 && reg <= Arm::ExtReg::S31) {
        size_t index = static_cast<size_t>(reg) - static_cast<size_t>(Arm::ExtReg::S0);
        return MDisp(R15, int(offsetof(JitState, ExtReg) + sizeof(u32) * index));
    }
    if (reg >= Arm::ExtReg::D0 && reg <= Arm::ExtReg::D31) {
        size_t index = static_cast<size_t>(reg) - static_cast<size_t>(Arm::ExtReg::D0);
        return MDisp(R15, int(offsetof(JitState, ExtReg) + sizeof(u64) * index));
    }
    ASSERT_MSG(false, "Should never happen.");
}

static OpArg MJitStateCpsr() {
    return MDisp(R15, offsetof(JitState, Cpsr));
}

static IR::Inst* FindUseWithOpcode(IR::Inst* inst, IR::Opcode opcode) {
    switch (opcode) {
        case IR::Opcode::GetCarryFromOp:
            return inst->carry_inst;
        case IR::Opcode::GetOverflowFromOp:
            return inst->overflow_inst;
        default:
            break;
    }

    ASSERT_MSG(false, "unreachable");
    return nullptr;
}

static void EraseInstruction(IR::Block& block, IR::Inst* inst) {
    block.instructions.erase(block.instructions.iterator_to(*inst));
}

EmitX64::BlockDescriptor* EmitX64::Emit(const Arm::LocationDescriptor descriptor, Dynarmic::IR::Block& block) {
    inhibit_emission.clear();
    reg_alloc.Reset();

    code->INT3();
    CodePtr code_ptr = code->GetCodePtr();
    basic_blocks[descriptor].code_ptr = code_ptr;

    EmitCondPrelude(block.cond, block.cond_failed, block.location);

    for (auto iter = block.instructions.begin(); iter != block.instructions.end(); ++iter) {
        IR::Inst* inst = &*iter;

        // Call the relevant Emit* member function.
        switch (inst->GetOpcode()) {

#define OPCODE(name, type, ...)                    \
            case IR::Opcode::name:                 \
                EmitX64::Emit##name(block, inst);  \
                break;
#include "frontend/ir/opcodes.inc"
#undef OPCODE

            default:
                ASSERT_MSG(false, "Invalid opcode %zu", static_cast<size_t>(inst->GetOpcode()));
                break;
        }

        reg_alloc.EndOfAllocScope();
    }

    EmitAddCycles(block.cycle_count);
    EmitTerminal(block.terminal, block.location);

    reg_alloc.AssertNoMoreUses();

    basic_blocks[descriptor].size = code->GetCodePtr() - code_ptr;
    return &basic_blocks[descriptor];
}

void EmitX64::EmitBreakpoint(IR::Block&, IR::Inst*) {
    code->INT3();
}

void EmitX64::EmitIdentity(IR::Block& block, IR::Inst* inst) {
    if (!inst->GetArg(0).IsImmediate()) {
        reg_alloc.RegisterAddDef(inst, inst->GetArg(0));
    }
}

void EmitX64::EmitGetRegister(IR::Block&, IR::Inst* inst) {
    Arm::Reg reg = inst->GetArg(0).GetRegRef();
    X64Reg result = reg_alloc.DefRegister(inst, any_gpr);
    code->MOV(32, R(result), MJitStateReg(reg));
}

void EmitX64::EmitGetExtendedRegister32(IR::Block& block, IR::Inst* inst) {
    Arm::ExtReg reg = inst->GetArg(0).GetExtRegRef();
    ASSERT(reg >= Arm::ExtReg::S0 && reg <= Arm::ExtReg::S31);

    X64Reg result = reg_alloc.DefRegister(inst, any_xmm);
    code->MOVSS(result, MJitStateExtReg(reg));
}

void EmitX64::EmitGetExtendedRegister64(IR::Block&, IR::Inst* inst) {
    Arm::ExtReg reg = inst->GetArg(0).GetExtRegRef();
    ASSERT(reg >= Arm::ExtReg::D0 && reg <= Arm::ExtReg::D31);
    X64Reg result = reg_alloc.DefRegister(inst, any_xmm);
    code->MOVSD(result, MJitStateExtReg(reg));
}

void EmitX64::EmitSetRegister(IR::Block&, IR::Inst* inst) {
    Arm::Reg reg = inst->GetArg(0).GetRegRef();
    IR::Value arg = inst->GetArg(1);
    if (arg.IsImmediate()) {
        code->MOV(32, MJitStateReg(reg), Imm32(arg.GetU32()));
    } else {
        X64Reg to_store = reg_alloc.UseRegister(arg.GetInst(), any_gpr);
        code->MOV(32, MJitStateReg(reg), R(to_store));
    }
}

void EmitX64::EmitSetExtendedRegister32(IR::Block&, IR::Inst* inst) {
    Arm::ExtReg reg = inst->GetArg(0).GetExtRegRef();
    ASSERT(reg >= Arm::ExtReg::S0 && reg <= Arm::ExtReg::S31);
    X64Reg source = reg_alloc.UseRegister(inst->GetArg(1), any_xmm);
    code->MOVSS(MJitStateExtReg(reg), source);
}

void EmitX64::EmitSetExtendedRegister64(IR::Block&, IR::Inst* inst) {
    Arm::ExtReg reg = inst->GetArg(0).GetExtRegRef();
    ASSERT(reg >= Arm::ExtReg::D0 && reg <= Arm::ExtReg::D31);
    X64Reg source = reg_alloc.UseRegister(inst->GetArg(1), any_xmm);
    code->MOVSD(MJitStateExtReg(reg), source);
}

void EmitX64::EmitGetNFlag(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.DefRegister(inst, any_gpr);
    code->MOV(32, R(result), MJitStateCpsr());
    code->SHR(32, R(result), Imm8(31));
}

void EmitX64::EmitSetNFlag(IR::Block&, IR::Inst* inst) {
    constexpr size_t flag_bit = 31;
    constexpr u32 flag_mask = 1u << flag_bit;
    IR::Value arg = inst->GetArg(0);
    if (arg.IsImmediate()) {
        if (arg.GetU1()) {
            code->OR(32, MJitStateCpsr(), Imm32(flag_mask));
        } else {
            code->AND(32, MJitStateCpsr(), Imm32(~flag_mask));
        }
    } else {
        X64Reg to_store = reg_alloc.UseScratchRegister(arg.GetInst(), any_gpr);

        code->SHL(32, R(to_store), Imm8(flag_bit));
        code->AND(32, MJitStateCpsr(), Imm32(~flag_mask));
        code->OR(32, MJitStateCpsr(), R(to_store));
    }
}

void EmitX64::EmitGetZFlag(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.DefRegister(inst, any_gpr);
    code->MOV(32, R(result), MJitStateCpsr());
    code->SHR(32, R(result), Imm8(30));
    code->AND(32, R(result), Imm32(1));
}

void EmitX64::EmitSetZFlag(IR::Block&, IR::Inst* inst) {
    constexpr size_t flag_bit = 30;
    constexpr u32 flag_mask = 1u << flag_bit;
    IR::Value arg = inst->GetArg(0);
    if (arg.IsImmediate()) {
        if (arg.GetU1()) {
            code->OR(32, MJitStateCpsr(), Imm32(flag_mask));
        } else {
            code->AND(32, MJitStateCpsr(), Imm32(~flag_mask));
        }
    } else {
        X64Reg to_store = reg_alloc.UseScratchRegister(arg.GetInst(), any_gpr);

        code->SHL(32, R(to_store), Imm8(flag_bit));
        code->AND(32, MJitStateCpsr(), Imm32(~flag_mask));
        code->OR(32, MJitStateCpsr(), R(to_store));
    }
}

void EmitX64::EmitGetCFlag(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.DefRegister(inst, any_gpr);
    code->MOV(32, R(result), MJitStateCpsr());
    code->SHR(32, R(result), Imm8(29));
    code->AND(32, R(result), Imm32(1));
}

void EmitX64::EmitSetCFlag(IR::Block&, IR::Inst* inst) {
    constexpr size_t flag_bit = 29;
    constexpr u32 flag_mask = 1u << flag_bit;
    IR::Value arg = inst->GetArg(0);
    if (arg.IsImmediate()) {
        if (arg.GetU1()) {
            code->OR(32, MJitStateCpsr(), Imm32(flag_mask));
        } else {
            code->AND(32, MJitStateCpsr(), Imm32(~flag_mask));
        }
    } else {
        X64Reg to_store = reg_alloc.UseScratchRegister(arg.GetInst(), any_gpr);

        code->SHL(32, R(to_store), Imm8(flag_bit));
        code->AND(32, MJitStateCpsr(), Imm32(~flag_mask));
        code->OR(32, MJitStateCpsr(), R(to_store));
    }
}

void EmitX64::EmitGetVFlag(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.DefRegister(inst, any_gpr);
    code->MOV(32, R(result), MJitStateCpsr());
    code->SHR(32, R(result), Imm8(28));
    code->AND(32, R(result), Imm32(1));
}

void EmitX64::EmitSetVFlag(IR::Block&, IR::Inst* inst) {
    constexpr size_t flag_bit = 28;
    constexpr u32 flag_mask = 1u << flag_bit;
    IR::Value arg = inst->GetArg(0);
    if (arg.IsImmediate()) {
        if (arg.GetU1()) {
            code->OR(32, MJitStateCpsr(), Imm32(flag_mask));
        } else {
            code->AND(32, MJitStateCpsr(), Imm32(~flag_mask));
        }
    } else {
        X64Reg to_store = reg_alloc.UseScratchRegister(arg.GetInst(), any_gpr);

        code->SHL(32, R(to_store), Imm8(flag_bit));
        code->AND(32, MJitStateCpsr(), Imm32(~flag_mask));
        code->OR(32, MJitStateCpsr(), R(to_store));
    }
}

void EmitX64::EmitBXWritePC(IR::Block&, IR::Inst* inst) {
    const u32 T_bit = 1 << 5;
    auto arg = inst->GetArg(0);

    // Pseudocode:
    // if (new_pc & 1) {
    //    new_pc &= 0xFFFFFFFE;
    //    cpsr.T = true;
    // } else {
    //    new_pc &= 0xFFFFFFFC;
    //    cpsr.T = false;
    // }

    if (arg.IsImmediate()) {
        u32 new_pc = arg.GetU32();
        if (Common::Bit<0>(new_pc)) {
            new_pc &= 0xFFFFFFFE;
            code->MOV(32, MJitStateReg(Arm::Reg::PC), Imm32(new_pc));
            code->OR(32, MJitStateCpsr(), Imm32(T_bit));
        } else {
            new_pc &= 0xFFFFFFFC;
            code->MOV(32, MJitStateReg(Arm::Reg::PC), Imm32(new_pc));
            code->AND(32, MJitStateCpsr(), Imm32(~T_bit));
        }
    } else {
        X64Reg new_pc = reg_alloc.UseScratchRegister(arg.GetInst(), any_gpr);
        X64Reg tmp1 = reg_alloc.ScratchRegister(any_gpr);
        X64Reg tmp2 = reg_alloc.ScratchRegister(any_gpr);

        code->MOV(32, R(tmp1), MJitStateCpsr());
        code->MOV(32, R(tmp2), R(tmp1));
        code->AND(32, R(tmp2), Imm32(~T_bit));         // CPSR.T = 0
        code->OR(32, R(tmp1), Imm32(T_bit));           // CPSR.T = 1
        code->TEST(8, R(new_pc), Imm8(1));
        code->CMOVcc(32, tmp1, R(tmp2), CC_E);         // CPSR.T = pc & 1
        code->MOV(32, MJitStateCpsr(), R(tmp1));
        code->LEA(32, tmp2, MComplex(new_pc, new_pc, 1, 0));
        code->OR(32, R(tmp2), Imm32(0xFFFFFFFC));      // tmp2 = pc & 1 ? 0xFFFFFFFE : 0xFFFFFFFC
        code->AND(32, R(new_pc), R(tmp2));
        code->MOV(32, MJitStateReg(Arm::Reg::PC), R(new_pc));
    }
}

void EmitX64::EmitCallSupervisor(IR::Block&, IR::Inst* inst) {
    auto imm32 = inst->GetArg(0);

    reg_alloc.HostCall(nullptr, imm32);

    code->STMXCSR(MDisp(R15, offsetof(JitState, guest_MXCSR)));
    code->LDMXCSR(MDisp(R15, offsetof(JitState, save_host_MXCSR)));
    code->ABI_CallFunction(reinterpret_cast<void*>(cb.CallSVC));
    code->STMXCSR(MDisp(R15, offsetof(JitState, save_host_MXCSR)));
    code->LDMXCSR(MDisp(R15, offsetof(JitState, guest_MXCSR)));
}

void EmitX64::EmitGetCarryFromOp(IR::Block&, IR::Inst*) {
    ASSERT_MSG(0, "should never happen");
}

void EmitX64::EmitGetOverflowFromOp(IR::Block&, IR::Inst*) {
    ASSERT_MSG(0, "should never happen");
}

void EmitX64::EmitPack2x32To1x64(IR::Block&, IR::Inst* inst) {
    OpArg lo;
    X64Reg result;
    if (inst->GetArg(0).IsImmediate()) {
        // TODO: Optimize
        result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
        lo = Gen::R(result);
    } else {
        std::tie(lo, result) = reg_alloc.UseDefOpArg(inst->GetArg(0), inst, any_gpr);
    }
    X64Reg hi = reg_alloc.UseScratchRegister(inst->GetArg(1), any_gpr);

    code->SHL(64, R(hi), Imm8(32));
    code->MOVZX(64, 32, result, lo);
    code->OR(64, R(result), R(hi));
}

void EmitX64::EmitLeastSignificantWord(IR::Block&, IR::Inst* inst) {
    reg_alloc.RegisterAddDef(inst, inst->GetArg(0));
}

void EmitX64::EmitMostSignificantWord(IR::Block& block, IR::Inst* inst) {
    auto carry_inst = FindUseWithOpcode(inst, IR::Opcode::GetCarryFromOp);
    auto result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

    code->SHR(64, R(result), Imm8(32));

    if (carry_inst) {
        EraseInstruction(block, carry_inst);
        reg_alloc.DecrementRemainingUses(inst);
        X64Reg carry = reg_alloc.DefRegister(carry_inst, any_gpr);

        code->SETcc(CC_C, R(carry));
    }
}

void EmitX64::EmitLeastSignificantHalf(IR::Block&, IR::Inst* inst) {
    reg_alloc.RegisterAddDef(inst, inst->GetArg(0));
}

void EmitX64::EmitLeastSignificantByte(IR::Block&, IR::Inst* inst) {
    reg_alloc.RegisterAddDef(inst, inst->GetArg(0));
}

void EmitX64::EmitMostSignificantBit(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

    // TODO: Flag optimization

    code->SHR(32, R(result), Imm8(31));
}

void EmitX64::EmitIsZero(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

    // TODO: Flag optimization

    code->TEST(32, R(result), R(result));
    code->SETcc(CCFlags::CC_E, R(result));
    code->MOVZX(32, 8, result, R(result));
}

void EmitX64::EmitIsZero64(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

    // TODO: Flag optimization

    code->TEST(64, R(result), R(result));
    code->SETcc(CCFlags::CC_E, R(result));
    code->MOVZX(32, 8, result, R(result));
}

void EmitX64::EmitLogicalShiftLeft(IR::Block& block, IR::Inst* inst) {
    auto carry_inst = FindUseWithOpcode(inst, IR::Opcode::GetCarryFromOp);

    // TODO: Consider using BMI2 instructions like SHLX when arm-in-host flags is implemented.

    if (!carry_inst) {
        if (!inst->GetArg(2).IsImmediate()) {
            // TODO: Remove redundant argument.
            reg_alloc.DecrementRemainingUses(inst->GetArg(2).GetInst());
        }

        auto shift_arg = inst->GetArg(1);

        if (shift_arg.IsImmediate()) {
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            u8 shift = shift_arg.GetU8();

            if (shift <= 31) {
                code->SHL(32, R(result), Imm8(shift));
            } else {
                code->XOR(32, R(result), R(result));
            }
        } else {
            X64Reg shift = reg_alloc.UseRegister(shift_arg.GetInst(), {HostLoc::RCX});
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg zero = reg_alloc.ScratchRegister(any_gpr);

            // The 32-bit x64 SHL instruction masks the shift count by 0x1F before performing the shift.
            // ARM differs from the behaviour: It does not mask the count, so shifts above 31 result in zeros.

            code->SHL(32, R(result), R(shift));
            code->XOR(32, R(zero), R(zero));
            code->CMP(8, R(shift), Imm8(32));
            code->CMOVcc(32, result, R(zero), CC_NB);
        }
    } else {
        EraseInstruction(block, carry_inst);
        reg_alloc.DecrementRemainingUses(inst);

        auto shift_arg = inst->GetArg(1);

        if (shift_arg.IsImmediate()) {
            u8 shift = shift_arg.GetU8();
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg carry = reg_alloc.UseDefRegister(inst->GetArg(2), carry_inst, any_gpr);

            if (shift == 0) {
                // There is nothing more to do.
            } else if (shift < 32) {
                code->BT(32, R(carry), Imm8(0));
                code->SHL(32, R(result), Imm8(shift));
                code->SETcc(CC_C, R(carry));
            } else if (shift > 32) {
                code->XOR(32, R(result), R(result));
                code->XOR(32, R(carry), R(carry));
            } else {
                code->MOV(32, R(carry), R(result));
                code->XOR(32, R(result), R(result));
                code->AND(32, R(carry), Imm32(1));
            }
        } else {
            X64Reg shift = reg_alloc.UseRegister(shift_arg.GetInst(), {HostLoc::RCX});
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg carry = reg_alloc.UseDefRegister(inst->GetArg(2), carry_inst, any_gpr);

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
}

void EmitX64::EmitLogicalShiftRight(IR::Block& block, IR::Inst* inst) {
    auto carry_inst = FindUseWithOpcode(inst, IR::Opcode::GetCarryFromOp);

    if (!carry_inst) {
        if (!inst->GetArg(2).IsImmediate()) {
            // TODO: Remove redundant argument.
            reg_alloc.DecrementRemainingUses(inst->GetArg(2).GetInst());
        }

        auto shift_arg = inst->GetArg(1);

        if (shift_arg.IsImmediate()) {
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            u8 shift = shift_arg.GetU8();

            if (shift <= 31) {
                code->SHR(32, R(result), Imm8(shift));
            } else {
                code->XOR(32, R(result), R(result));
            }
        } else {
            X64Reg shift = reg_alloc.UseRegister(shift_arg.GetInst(), {HostLoc::RCX});
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg zero = reg_alloc.ScratchRegister(any_gpr);

            // The 32-bit x64 SHR instruction masks the shift count by 0x1F before performing the shift.
            // ARM differs from the behaviour: It does not mask the count, so shifts above 31 result in zeros.

            code->SHR(32, R(result), R(shift));
            code->XOR(32, R(zero), R(zero));
            code->CMP(8, R(shift), Imm8(32));
            code->CMOVcc(32, result, R(zero), CC_NB);
        }
    } else {
        EraseInstruction(block, carry_inst);
        reg_alloc.DecrementRemainingUses(inst);

        auto shift_arg = inst->GetArg(1);

        if (shift_arg.IsImmediate()) {
            u8 shift = shift_arg.GetU8();
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg carry = reg_alloc.UseDefRegister(inst->GetArg(2), carry_inst, any_gpr);

            if (shift == 0) {
                // There is nothing more to do.
            } else if (shift < 32) {
                code->SHR(32, R(result), Imm8(shift));
                code->SETcc(CC_C, R(carry));
            } else if (shift == 32) {
                code->BT(32, R(result), Imm8(31));
                code->SETcc(CC_C, R(carry));
                code->MOV(32, R(result), Imm32(0));
            } else {
                code->XOR(32, R(result), R(result));
                code->XOR(32, R(carry), R(carry));
            }
        } else {
            X64Reg shift = reg_alloc.UseRegister(shift_arg.GetInst(), {HostLoc::RCX});
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg carry = reg_alloc.UseDefRegister(inst->GetArg(2), carry_inst, any_gpr);

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
            code->XOR(32, R(result), R(result));
            code->XOR(32, R(carry), R(carry));
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
}

void EmitX64::EmitArithmeticShiftRight(IR::Block& block, IR::Inst* inst) {
    auto carry_inst = FindUseWithOpcode(inst, IR::Opcode::GetCarryFromOp);

    if (!carry_inst) {
        if (!inst->GetArg(2).IsImmediate()) {
            // TODO: Remove redundant argument.
            reg_alloc.DecrementRemainingUses(inst->GetArg(2).GetInst());
        }

        auto shift_arg = inst->GetArg(1);

        if (shift_arg.IsImmediate()) {
            u8 shift = shift_arg.GetU8();
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

            code->SAR(32, R(result), Imm8(shift < 31 ? shift : 31));
        } else {
            X64Reg shift = reg_alloc.UseScratchRegister(shift_arg.GetInst(), {HostLoc::RCX});
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg const31 = reg_alloc.ScratchRegister(any_gpr);

            // The 32-bit x64 SAR instruction masks the shift count by 0x1F before performing the shift.
            // ARM differs from the behaviour: It does not mask the count.

            // We note that all shift values above 31 have the same behaviour as 31 does, so we saturate `shift` to 31.
            code->MOV(32, R(const31), Imm32(31));
            code->MOVZX(32, 8, shift, R(shift));
            code->CMP(32, R(shift), Imm32(31));
            code->CMOVcc(32, shift, R(const31), CC_G);
            code->SAR(32, R(result), R(shift));
        }
    } else {
        EraseInstruction(block, carry_inst);
        reg_alloc.DecrementRemainingUses(inst);

        auto shift_arg = inst->GetArg(1);

        if (shift_arg.IsImmediate()) {
            u8 shift = shift_arg.GetU8();
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg carry = reg_alloc.UseDefRegister(inst->GetArg(2), carry_inst, any_gpr);

            if (shift == 0) {
                // There is nothing more to do.
            } else if (shift <= 31) {
                code->SAR(32, R(result), Imm8(shift));
                code->SETcc(CC_C, R(carry));
            } else {
                code->SAR(32, R(result), Imm8(31));
                code->BT(32, R(result), Imm8(31));
                code->SETcc(CC_C, R(carry));
            }
        } else {
            X64Reg shift = reg_alloc.UseRegister(shift_arg.GetInst(), {HostLoc::RCX});
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg carry = reg_alloc.UseDefRegister(inst->GetArg(2), carry_inst, any_gpr);

            // TODO: Optimize this.

            code->CMP(8, R(shift), Imm8(31));
            auto Rs_gt31 = code->J_CC(CC_A);
            // if (Rs & 0xFF == 0) goto end;
            code->TEST(8, R(shift), R(shift));
            auto Rs_zero = code->J_CC(CC_Z);
            // if (Rs & 0xFF <= 31) {
            code->SAR(32, R(result), R(shift));
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
}

void EmitX64::EmitRotateRight(IR::Block& block, IR::Inst* inst) {
    auto carry_inst = FindUseWithOpcode(inst, IR::Opcode::GetCarryFromOp);

    if (!carry_inst) {
        if (!inst->GetArg(2).IsImmediate()) {
            // TODO: Remove redundant argument.
            reg_alloc.DecrementRemainingUses(inst->GetArg(2).GetInst());
        }

        auto shift_arg = inst->GetArg(1);

        if (shift_arg.IsImmediate()) {
            u8 shift = shift_arg.GetU8();
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

            code->ROR(32, R(result), Imm8(shift & 0x1F));
        } else {
            X64Reg shift = reg_alloc.UseRegister(shift_arg.GetInst(), {HostLoc::RCX});
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

            // x64 ROR instruction does (shift & 0x1F) for us.
            code->ROR(32, R(result), R(shift));
        }
    } else {
        EraseInstruction(block, carry_inst);
        reg_alloc.DecrementRemainingUses(inst);

        auto shift_arg = inst->GetArg(1);

        if (shift_arg.IsImmediate()) {
            u8 shift = shift_arg.GetU8();
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg carry = reg_alloc.UseDefRegister(inst->GetArg(2), carry_inst, any_gpr);

            if (shift == 0) {
                // There is nothing more to do.
            } else if ((shift & 0x1F) == 0) {
                code->BT(32, R(result), Imm8(31));
                code->SETcc(CC_C, R(carry));
            } else {
                code->ROR(32, R(result), Imm8(shift));
                code->SETcc(CC_C, R(carry));
            }
        } else {
            X64Reg shift = reg_alloc.UseScratchRegister(shift_arg.GetInst(), {HostLoc::RCX});
            X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
            X64Reg carry = reg_alloc.UseDefRegister(inst->GetArg(2), carry_inst, any_gpr);

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
}

void EmitX64::EmitRotateRightExtended(IR::Block& block, IR::Inst* inst) {
    auto carry_inst = FindUseWithOpcode(inst, IR::Opcode::GetCarryFromOp);

    X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
    X64Reg carry = carry_inst
                    ? reg_alloc.UseDefRegister(inst->GetArg(1), carry_inst, any_gpr)
                    : reg_alloc.UseRegister(inst->GetArg(1), any_gpr);

    code->BT(32, R(carry), Imm8(0));
    code->RCR(32, R(result), Imm8(1));

    if (carry_inst) {
        EraseInstruction(block, carry_inst);
        reg_alloc.DecrementRemainingUses(inst);
        code->SETcc(CC_C, R(carry));
    }
}

static X64Reg DoCarry(RegAlloc& reg_alloc, const IR::Value& carry_in, IR::Inst* carry_out) {
    if (carry_in.IsImmediate()) {
        return carry_out ? reg_alloc.DefRegister(carry_out, any_gpr) : INVALID_REG;
    } else {
        IR::Inst* in = carry_in.GetInst();
        return carry_out ? reg_alloc.UseDefRegister(in, carry_out, any_gpr) : reg_alloc.UseRegister(in, any_gpr);
    }
}

void EmitX64::EmitAddWithCarry(IR::Block& block, IR::Inst* inst) {
    auto carry_inst = FindUseWithOpcode(inst, IR::Opcode::GetCarryFromOp);
    auto overflow_inst = FindUseWithOpcode(inst, IR::Opcode::GetOverflowFromOp);

    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);
    IR::Value carry_in = inst->GetArg(2);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_gpr);
    X64Reg carry = DoCarry(reg_alloc, carry_in, carry_inst);
    X64Reg overflow = overflow_inst ? reg_alloc.DefRegister(overflow_inst, any_gpr) : INVALID_REG;

    // TODO: Consider using LEA.

    OpArg op_arg = reg_alloc.UseOpArg(b, any_gpr);

    if (carry_in.IsImmediate()) {
        if (carry_in.GetU1()) {
            code->STC();
            code->ADC(32, R(result), op_arg);
        } else {
            code->ADD(32, R(result), op_arg);
        }
    } else {
        code->BT(32, R(carry), Imm8(0));
        code->ADC(32, R(result), op_arg);
    }

    if (carry_inst) {
        EraseInstruction(block, carry_inst);
        reg_alloc.DecrementRemainingUses(inst);
        code->SETcc(Gen::CC_C, R(carry));
    }
    if (overflow_inst) {
        EraseInstruction(block, overflow_inst);
        reg_alloc.DecrementRemainingUses(inst);
        code->SETcc(Gen::CC_O, R(overflow));
    }
}

void EmitX64::EmitAdd64(IR::Block& block, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_gpr);
    OpArg op_arg = reg_alloc.UseOpArg(b, any_gpr);

    code->ADD(64, R(result), op_arg);
}

void EmitX64::EmitSubWithCarry(IR::Block& block, IR::Inst* inst) {
    auto carry_inst = FindUseWithOpcode(inst, IR::Opcode::GetCarryFromOp);
    auto overflow_inst = FindUseWithOpcode(inst, IR::Opcode::GetOverflowFromOp);

    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);
    IR::Value carry_in = inst->GetArg(2);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_gpr);
    X64Reg carry = DoCarry(reg_alloc, carry_in, carry_inst);
    X64Reg overflow = overflow_inst ? reg_alloc.DefRegister(overflow_inst, any_gpr) : INVALID_REG;

    // TODO: Consider using LEA.
    // TODO: Optimize CMP case.
    // Note that x64 CF is inverse of what the ARM carry flag is here.

    OpArg op_arg = reg_alloc.UseOpArg(b, any_gpr);

    if (carry_in.IsImmediate()) {
        if (carry_in.GetU1()) {
            code->SUB(32, R(result), op_arg);
        } else {
            code->STC();
            code->SBB(32, R(result), op_arg);
        }
    } else {
        code->BT(32, R(carry), Imm8(0));
        code->CMC();
        code->SBB(32, R(result), op_arg);
    }

    if (carry_inst) {
        EraseInstruction(block, carry_inst);
        reg_alloc.DecrementRemainingUses(inst);
        code->SETcc(Gen::CC_NC, R(carry));
    }
    if (overflow_inst) {
        EraseInstruction(block, overflow_inst);
        reg_alloc.DecrementRemainingUses(inst);
        code->SETcc(Gen::CC_O, R(overflow));
    }
}

void EmitX64::EmitSub64(IR::Block& block, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_gpr);
    OpArg op_arg = reg_alloc.UseOpArg(b, any_gpr);

    code->SUB(64, R(result), op_arg);
}

void EmitX64::EmitMul(IR::Block&, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);
    if (a.IsImmediate())
        std::swap(a, b);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_gpr);
    if (b.IsImmediate()) {
        code->IMUL(32, result, R(result), Imm32(b.GetU32()));
    } else {
        OpArg op_arg = reg_alloc.UseOpArg(b, any_gpr);
        code->IMUL(32, result, op_arg);
    }
}

void EmitX64::EmitMul64(IR::Block&, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_gpr);
    OpArg op_arg = reg_alloc.UseOpArg(b, any_gpr);

    code->IMUL(64, result, op_arg);
}

void EmitX64::EmitAnd(IR::Block&, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_gpr);
    OpArg op_arg = reg_alloc.UseOpArg(b, any_gpr);

    code->AND(32, R(result), op_arg);
}

void EmitX64::EmitEor(IR::Block&, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_gpr);
    OpArg op_arg = reg_alloc.UseOpArg(b, any_gpr);

    code->XOR(32, R(result), op_arg);
}

void EmitX64::EmitOr(IR::Block&, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_gpr);
    OpArg op_arg = reg_alloc.UseOpArg(b, any_gpr);

    code->OR(32, R(result), op_arg);
}

void EmitX64::EmitNot(IR::Block&, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);

    if (a.IsImmediate()) {
        X64Reg result = reg_alloc.DefRegister(inst, any_gpr);

        code->MOV(32, R(result), Imm32(~a.GetU32()));
    } else {
        X64Reg result = reg_alloc.UseDefRegister(a.GetInst(), inst, any_gpr);

        code->NOT(32, R(result));
    }
}

void EmitX64::EmitSignExtendWordToLong(IR::Block&, IR::Inst* inst) {
    OpArg source;
    X64Reg result;
    if (inst->GetArg(0).IsImmediate()) {
        // TODO: Optimize
        result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
        source = Gen::R(result);
    } else {
        std::tie(source, result) = reg_alloc.UseDefOpArg(inst->GetArg(0), inst, any_gpr);
    }

    code->MOVSX(64, 32, result, source);
}

void EmitX64::EmitSignExtendHalfToWord(IR::Block&, IR::Inst* inst) {
    OpArg source;
    X64Reg result;
    if (inst->GetArg(0).IsImmediate()) {
        // TODO: Optimize
        result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
        source = Gen::R(result);
    } else {
        std::tie(source, result) = reg_alloc.UseDefOpArg(inst->GetArg(0), inst, any_gpr);
    }

    code->MOVSX(32, 16, result, source);
}

void EmitX64::EmitSignExtendByteToWord(IR::Block&, IR::Inst* inst) {
    OpArg source;
    X64Reg result;
    if (inst->GetArg(0).IsImmediate()) {
        // TODO: Optimize
        result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
        source = Gen::R(result);
    } else {
        std::tie(source, result) = reg_alloc.UseDefOpArg(inst->GetArg(0), inst, any_gpr);
    }

    code->MOVSX(32, 8, result, source);
}

void EmitX64::EmitZeroExtendWordToLong(IR::Block&, IR::Inst* inst) {
    OpArg source;
    X64Reg result;
    if (inst->GetArg(0).IsImmediate()) {
        // TODO: Optimize
        result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
        source = Gen::R(result);
    } else {
        std::tie(source, result) = reg_alloc.UseDefOpArg(inst->GetArg(0), inst, any_gpr);
    }

    code->MOVZX(64, 32, result, source);
}

void EmitX64::EmitZeroExtendHalfToWord(IR::Block&, IR::Inst* inst) {
    OpArg source;
    X64Reg result;
    if (inst->GetArg(0).IsImmediate()) {
        // TODO: Optimize
        result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
        source = Gen::R(result);
    } else {
        std::tie(source, result) = reg_alloc.UseDefOpArg(inst->GetArg(0), inst, any_gpr);
    }

    code->MOVZX(32, 16, result, source);
}

void EmitX64::EmitZeroExtendByteToWord(IR::Block&, IR::Inst* inst) {
    OpArg source;
    X64Reg result;
    if (inst->GetArg(0).IsImmediate()) {
        // TODO: Optimize
        result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);
        source = Gen::R(result);
    } else {
        std::tie(source, result) = reg_alloc.UseDefOpArg(inst->GetArg(0), inst, any_gpr);
    }

    code->MOVZX(32, 8, result, source);
}

void EmitX64::EmitByteReverseWord(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

    code->BSWAP(32, result);
}

void EmitX64::EmitByteReverseHalf(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

    code->ROL(16, R(result), Imm8(8));
}

void EmitX64::EmitByteReverseDual(IR::Block&, IR::Inst* inst) {
    X64Reg result = reg_alloc.UseDefRegister(inst->GetArg(0), inst, any_gpr);

    code->BSWAP(64, result);
}

static void DenormalsAreZero32(XEmitter* code, X64Reg xmm_value, X64Reg gpr_scratch) {
    // We need to report back whether we've found a denormal on input.
    // SSE doesn't do this for us when SSE's DAZ is enabled.
    code->MOVD_xmm(R(gpr_scratch), xmm_value);
    code->AND(32, R(gpr_scratch), Imm32(0x7FFFFFFF));
    code->SUB(32, R(gpr_scratch), Imm32(1));
    code->CMP(32, R(gpr_scratch), Imm32(0x007FFFFE));
    auto fixup = code->J_CC(CC_A);
    code->PXOR(xmm_value, R(xmm_value));
    code->MOV(32, MDisp(R15, offsetof(JitState, FPSCR_IDC)), Imm32(1 << 7));
    code->SetJumpTarget(fixup);
}

static void DenormalsAreZero64(XEmitter* code, Routines* routines, X64Reg xmm_value, X64Reg gpr_scratch) {
    code->MOVQ_xmm(R(gpr_scratch), xmm_value);
    code->AND(64, R(gpr_scratch), routines->MFloatNonSignMask64());
    code->SUB(64, R(gpr_scratch), Imm32(1));
    code->CMP(64, R(gpr_scratch), routines->MFloatPenultimatePositiveDenormal64());
    auto fixup = code->J_CC(CC_A);
    code->PXOR(xmm_value, R(xmm_value));
    code->MOV(32, MDisp(R15, offsetof(JitState, FPSCR_IDC)), Imm32(1 << 7));
    code->SetJumpTarget(fixup);
}

static void FlushToZero32(XEmitter* code, X64Reg xmm_value, X64Reg gpr_scratch) {
    code->MOVD_xmm(R(gpr_scratch), xmm_value);
    code->AND(32, R(gpr_scratch), Imm32(0x7FFFFFFF));
    code->SUB(32, R(gpr_scratch), Imm32(1));
    code->CMP(32, R(gpr_scratch), Imm32(0x007FFFFE));
    auto fixup = code->J_CC(CC_A);
    code->PXOR(xmm_value, R(xmm_value));
    code->MOV(32, MDisp(R15, offsetof(JitState, FPSCR_UFC)), Imm32(1 << 3));
    code->SetJumpTarget(fixup);
}

static void FlushToZero64(XEmitter* code, Routines* routines, X64Reg xmm_value, X64Reg gpr_scratch) {
    code->MOVQ_xmm(R(gpr_scratch), xmm_value);
    code->AND(64, R(gpr_scratch), routines->MFloatNonSignMask64());
    code->SUB(64, R(gpr_scratch), Imm32(1));
    code->CMP(64, R(gpr_scratch), routines->MFloatPenultimatePositiveDenormal64());
    auto fixup = code->J_CC(CC_A);
    code->PXOR(xmm_value, R(xmm_value));
    code->MOV(32, MDisp(R15, offsetof(JitState, FPSCR_UFC)), Imm32(1 << 3));
    code->SetJumpTarget(fixup);
}

static void DefaultNaN32(XEmitter* code, Routines* routines, X64Reg xmm_value) {
    code->UCOMISS(xmm_value, R(xmm_value));
    auto fixup = code->J_CC(CC_NP);
    code->MOVAPS(xmm_value, routines->MFloatNaN32());
    code->SetJumpTarget(fixup);
}

static void DefaultNaN64(XEmitter* code, Routines* routines, X64Reg xmm_value) {
    code->UCOMISD(xmm_value, R(xmm_value));
    auto fixup = code->J_CC(CC_NP);
    code->MOVAPS(xmm_value, routines->MFloatNaN64());
    code->SetJumpTarget(fixup);
}

void EmitX64::EmitFPAdd32(IR::Block& block, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_xmm);
    X64Reg operand = reg_alloc.UseRegister(b, any_xmm);
    X64Reg gpr_scratch = reg_alloc.ScratchRegister(any_gpr);

    if (block.location.FPSCR_FTZ()) {
        DenormalsAreZero32(code, result, gpr_scratch);
        DenormalsAreZero32(code, operand, gpr_scratch);
    }
    code->ADDSS(result, R(operand));
    if (block.location.FPSCR_FTZ()) {
        FlushToZero32(code, result, gpr_scratch);
    }
    if (block.location.FPSCR_DN()) {
        DefaultNaN32(code, routines, result);
    }
}

void EmitX64::EmitFPAdd64(IR::Block& block, IR::Inst* inst) {
    IR::Value a = inst->GetArg(0);
    IR::Value b = inst->GetArg(1);

    X64Reg result = reg_alloc.UseDefRegister(a, inst, any_xmm);
    X64Reg operand = reg_alloc.UseRegister(b, any_xmm);
    X64Reg gpr_scratch = reg_alloc.ScratchRegister(any_gpr);

    if (block.location.FPSCR_FTZ()) {
        DenormalsAreZero64(code, routines, result, gpr_scratch);
        DenormalsAreZero64(code, routines, operand, gpr_scratch);
    }
    code->ADDSD(result, R(operand));
    if (block.location.FPSCR_FTZ()) {
        FlushToZero64(code, routines, result, gpr_scratch);
    }
    if (block.location.FPSCR_DN()) {
        DefaultNaN64(code, routines, result);
    }
}

void EmitX64::EmitReadMemory8(IR::Block&, IR::Inst* inst) {
    reg_alloc.HostCall(inst, inst->GetArg(0));

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryRead8));
}

void EmitX64::EmitReadMemory16(IR::Block&, IR::Inst* inst) {
    reg_alloc.HostCall(inst, inst->GetArg(0));

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryRead16));
}

void EmitX64::EmitReadMemory32(IR::Block&, IR::Inst* inst) {
    reg_alloc.HostCall(inst, inst->GetArg(0));

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryRead32));
}

void EmitX64::EmitReadMemory64(IR::Block&, IR::Inst* inst) {
    reg_alloc.HostCall(inst, inst->GetArg(0));

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryRead64));
}

void EmitX64::EmitWriteMemory8(IR::Block&, IR::Inst* inst) {
    reg_alloc.HostCall(nullptr, inst->GetArg(0), inst->GetArg(1));

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryWrite8));
}

void EmitX64::EmitWriteMemory16(IR::Block&, IR::Inst* inst) {
    reg_alloc.HostCall(nullptr, inst->GetArg(0), inst->GetArg(1));

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryWrite16));
}

void EmitX64::EmitWriteMemory32(IR::Block&, IR::Inst* inst) {
    reg_alloc.HostCall(nullptr, inst->GetArg(0), inst->GetArg(1));

    code->ABI_CallFunction(reinterpret_cast<void*>(cb.MemoryWrite32));
}

void EmitX64::EmitWriteMemory64(IR::Block&, IR::Inst* inst) {
    reg_alloc.HostCall(nullptr, inst->GetArg(0), inst->GetArg(1));

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
    auto fixup = code->J_CC(cc, true);
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
    ASSERT_MSG(terminal.next.TFlag() == initial_location.TFlag(), "Unimplemented");
    ASSERT_MSG(terminal.next.EFlag() == initial_location.EFlag(), "Unimplemented");

    code->MOV(64, R(ABI_PARAM1), Imm64(terminal.next.PC()));
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
    code->MOV(32, MJitStateReg(Arm::Reg::PC), Imm32(terminal.next.PC()));
    if (terminal.next.TFlag() != initial_location.TFlag()) {
        if (terminal.next.TFlag()) {
            code->OR(32, MJitStateCpsr(), Imm32(1 << 5));
        } else {
            code->AND(32, MJitStateCpsr(), Imm32(~(1 << 5)));
        }
    }
    if (terminal.next.EFlag() != initial_location.EFlag()) {
        if (terminal.next.EFlag()) {
            code->OR(32, MJitStateCpsr(), Imm32(1 << 9));
        } else {
            code->AND(32, MJitStateCpsr(), Imm32(~(1 << 9)));
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
    auto fixup = code->J_CC(cc, true);
    EmitTerminal(terminal.else_, initial_location);
    code->SetJumpTarget(fixup);
    EmitTerminal(terminal.then_, initial_location);
}

void EmitX64::ClearCache() {
    basic_blocks.clear();
}

} // namespace BackendX64
} // namespace Dynarmic
