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
#include "translate.h"

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

    bool TranslateThisInstruction() {
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
        return TranslateThisInstruction();
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
