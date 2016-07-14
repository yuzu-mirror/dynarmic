/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/arm_types.h"
#include "frontend/decoder/arm.h"
#include "frontend/ir/ir.h"
#include "frontend/ir_emitter.h"
#include "frontend/translate.h"

namespace Dynarmic {
namespace Arm {

namespace {

struct ArmTranslatorVisitor final {
    explicit ArmTranslatorVisitor(LocationDescriptor descriptor) : ir(descriptor) {
        ASSERT_MSG(!descriptor.TFlag, "The processor must be in Arm mode");
    }

    IREmitter ir;

    bool TranslateThisInstruction() {
        ir.SetTerm(IR::Term::Interpret(ir.current_location));
        return false;
    }

    bool UnpredictableInstruction() {
        ASSERT_MSG(false, "UNPREDICTABLE");
        return false;
    }

    bool arm_UDF() {
        return TranslateThisInstruction();
    }
};

} // local namespace

IR::Block TranslateArm(LocationDescriptor descriptor, MemoryRead32FuncType memory_read_32) {
    ArmTranslatorVisitor visitor{descriptor};

    bool should_continue = true;
    while (should_continue) {
        const u32 arm_pc = visitor.ir.current_location.arm_pc;
        const u32 arm_instruction = (*memory_read_32)(arm_pc);

        const auto decoder = DecodeArm<ArmTranslatorVisitor>(arm_instruction);
        if (decoder) {
            should_continue = decoder->call(visitor, arm_instruction);
        } else {
            should_continue = visitor.arm_UDF();
        }

        visitor.ir.current_location.arm_pc += 4;
        visitor.ir.block.cycle_count++;
    }

    return visitor.ir.block;
}

} // namespace Arm
} // namespace Dynarmic
