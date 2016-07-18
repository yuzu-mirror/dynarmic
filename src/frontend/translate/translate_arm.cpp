/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/arm_types.h"
#include "frontend/decoder/arm.h"
#include "frontend/ir/ir.h"
#include "frontend/ir/ir_emitter.h"
#include "frontend/translate/translate.h"

namespace Dynarmic {
namespace Arm {

namespace {

enum class ConditionalState {
    /// We haven't met any conditional instructions yet.
    None,
    /// Current instruction is a conditional. This marks the end of this basic block.
    Break,
    /// This basic block is made up solely of conditional instructions.
    Translating,
};

struct ArmTranslatorVisitor final {
    explicit ArmTranslatorVisitor(LocationDescriptor descriptor) : ir(descriptor) {
        ASSERT_MSG(!descriptor.TFlag, "The processor must be in Arm mode");
    }

    IREmitter ir;
    ConditionalState cond_state = ConditionalState::None;

    bool InterpretThisInstruction() {
        ir.SetTerm(IR::Term::Interpret(ir.current_location));
        return false;
    }

    bool UnpredictableInstruction() {
        ASSERT_MSG(false, "UNPREDICTABLE");
        return false;
    }

    bool LinkToNextInstruction() {
        auto next_location = ir.current_location;
        next_location.arm_pc += 4;
        ir.SetTerm(IR::Term::LinkBlock{next_location});
        return false;
    }

    bool ConditionPassed(Cond cond) {
        ASSERT_MSG(cond_state != ConditionalState::Translating,
                   "In the current impl, ConditionPassed should never be called again once a non-AL cond is hit. "
                   "(i.e.: one and only one conditional instruction per block)");
        ASSERT_MSG(cond_state != ConditionalState::Break,
                   "This should never happen. We requested a break but that wasn't honored.");
        ASSERT_MSG(cond != Cond::NV, "NV conditional is obsolete");

        if (cond == Cond::AL) {
            // Everything is fine with the world
            return true;
        }

        // non-AL cond

        if (!ir.block.instructions.empty()) {
            // We've already emitted instructions. Quit for now, we'll make a new block here later.
            cond_state = ConditionalState::Break;
            ir.SetTerm(IR::Term::LinkBlock{ir.current_location});
            return false;
        }

        // We've not emitted instructions yet.
        // We'll emit one instruction, and set the block-entry conditional appropriately.

        cond_state = ConditionalState::Translating;
        ir.block.cond = cond;
        return true;
    }

    u32 rotr(u32 x, int shift) {
        shift &= 31;
        if (!shift) return x;
        return (x >> shift) | (x << (32 - shift));
    }

    u32 ArmExpandImm(int rotate, Imm8 imm8) {
        return rotr(static_cast<u32>(imm8), rotate*2);
    }

    bool arm_ADC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        u32 imm32 = ArmExpandImm(rotate, imm8);
        // ADC{S}<c> <Rd>, <Rn>, #<imm>
        if (ConditionPassed(cond)) {
            auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.GetCFlag());

            if (d == Reg::PC) {
                ASSERT(!S);
                ir.ALUWritePC(result.result);
                ir.SetTerm(IR::Term::ReturnToDispatch{});
                return false;
            }

            ir.SetRegister(d, result.result);
            if (S) {
                ir.SetNFlag(ir.MostSignificantBit(result.result));
                ir.SetZFlag(ir.IsZero(result.result));
                ir.SetCFlag(result.carry);
                ir.SetVFlag(result.overflow);
            }
        }
        return true;
    };

    bool arm_ADC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_ADC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_ADD_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_ADD_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_ADD_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_AND_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_AND_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_AND_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_BIC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_BIC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_BIC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_CMN_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_CMN_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_CMN_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }

    bool arm_CMP_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        u32 imm32 = ArmExpandImm(rotate, imm8);
        // CMP<c> <Rn>, #<imm>
        if (ConditionPassed(cond)) {
            auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(~imm32), ir.Imm1(true));
            ir.SetNFlag(ir.MostSignificantBit(result.result));
            ir.SetZFlag(ir.IsZero(result.result));
            ir.SetCFlag(result.carry);
            ir.SetVFlag(result.overflow);
        }
        return true;
    }

    bool arm_CMP_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_CMP_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_EOR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_EOR_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_EOR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_MOV_imm(Cond cond, bool S, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_MOV_reg(Cond cond, bool S, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_MOV_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_MVN_imm(Cond cond, bool S, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_MVN_reg(Cond cond, bool S, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_MVN_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_ORR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_ORR_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_ORR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_RSB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_RSB_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_RSB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_RSC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_RSC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_RSC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_SBC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_SBC_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_SBC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_SUB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_SUB_reg(Cond cond, bool S, Reg n, Reg d, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_SUB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_TEQ_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_TEQ_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_TEQ_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_TST_imm(Cond cond, Reg n, int rotate, Imm8 imm8) {
        return InterpretThisInstruction();
    }
    bool arm_TST_reg(Cond cond, Reg n, Imm5 imm5, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }
    bool arm_TST_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
        return InterpretThisInstruction();
    }

    bool arm_SVC(Cond cond, Imm24 imm24) {
        u32 imm32 = imm24;
        // SVC<c> #<imm24>
        if (ConditionPassed(cond)) {
            ir.CallSupervisor(ir.Imm32(imm32));
            return LinkToNextInstruction();
        }
        return true;
    }

    bool arm_UDF() {
        return InterpretThisInstruction();
    }
};

} // local namespace

IR::Block TranslateArm(LocationDescriptor descriptor, MemoryRead32FuncType memory_read_32) {
    ArmTranslatorVisitor visitor{descriptor};

    bool should_continue = true;
    while (should_continue && visitor.cond_state == ConditionalState::None) {
        const u32 arm_pc = visitor.ir.current_location.arm_pc;
        const u32 arm_instruction = (*memory_read_32)(arm_pc);

        const auto decoder = DecodeArm<ArmTranslatorVisitor>(arm_instruction);
        if (decoder) {
            should_continue = decoder->call(visitor, arm_instruction);
        } else {
            should_continue = visitor.arm_UDF();
        }

        if (visitor.cond_state == ConditionalState::Break) {
            break;
        }

        visitor.ir.current_location.arm_pc += 4;
        visitor.ir.block.cycle_count++;
    }

    if (visitor.cond_state == ConditionalState::Translating) {
        if (should_continue) {
            visitor.ir.SetTerm(IR::Term::LinkBlock{visitor.ir.current_location});
        }
        visitor.ir.block.cond_failed = { visitor.ir.current_location };
    }

    return visitor.ir.block;
}

} // namespace Arm
} // namespace Dynarmic
