/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <tuple>

#include <dynarmic/A32/config.h>

#include "common/assert.h"
#include "common/bit_util.h"
#include "frontend/imm.h"
#include "frontend/A32/decoder/thumb16.h"
#include "frontend/A32/decoder/thumb32.h"
#include "frontend/A32/ir_emitter.h"
#include "frontend/A32/location_descriptor.h"
#include "frontend/A32/translate/impl/translate_thumb.h"
#include "frontend/A32/translate/translate.h"

namespace Dynarmic::A32 {
namespace {

enum class ThumbInstSize {
    Thumb16, Thumb32
};

bool IsThumb16(u16 first_part) {
    return (first_part & 0xF800) <= 0xE800;
}

std::tuple<u32, ThumbInstSize> ReadThumbInstruction(u32 arm_pc, MemoryReadCodeFuncType memory_read_code) {
    u32 first_part = memory_read_code(arm_pc & 0xFFFFFFFC);
    if ((arm_pc & 0x2) != 0) {
        first_part >>= 16;
    }
    first_part &= 0xFFFF;

    if (IsThumb16(static_cast<u16>(first_part))) {
        // 16-bit thumb instruction
        return std::make_tuple(first_part, ThumbInstSize::Thumb16);
    }

    // 32-bit thumb instruction
    // These always start with 0b11101, 0b11110 or 0b11111.

    u32 second_part = memory_read_code((arm_pc + 2) & 0xFFFFFFFC);
    if (((arm_pc + 2) & 0x2) != 0) {
        second_part >>= 16;
    }
    second_part &= 0xFFFF;

    return std::make_tuple(static_cast<u32>((first_part << 16) | second_part), ThumbInstSize::Thumb32);
}

} // local namespace

IR::Block TranslateThumb(LocationDescriptor descriptor, MemoryReadCodeFuncType memory_read_code, const TranslationOptions& options) {
    const bool single_step = descriptor.SingleStepping();

    IR::Block block{descriptor};
    ThumbTranslatorVisitor visitor{block, descriptor, options};

    bool should_continue = true;
    do {
        const u32 arm_pc = visitor.ir.current_location.PC();
        const auto [thumb_instruction, inst_size] = ReadThumbInstruction(arm_pc, memory_read_code);

        if (inst_size == ThumbInstSize::Thumb16) {
            if (const auto decoder = DecodeThumb16<ThumbTranslatorVisitor>(static_cast<u16>(thumb_instruction))) {
                should_continue = decoder->get().call(visitor, static_cast<u16>(thumb_instruction));
            } else {
                should_continue = visitor.thumb16_UDF();
            }
        } else {
            if (const auto decoder = DecodeThumb32<ThumbTranslatorVisitor>(thumb_instruction)) {
                should_continue = decoder->get().call(visitor, thumb_instruction);
            } else {
                should_continue = visitor.thumb32_UDF();
            }
        }

        const s32 advance_pc = (inst_size == ThumbInstSize::Thumb16) ? 2 : 4;
        visitor.ir.current_location = visitor.ir.current_location.AdvancePC(advance_pc);
        block.CycleCount()++;
    } while (should_continue && !single_step);

    if (single_step && should_continue) {
        visitor.ir.SetTerm(IR::Term::LinkBlock{visitor.ir.current_location});
    }

    block.SetEndLocation(visitor.ir.current_location);

    return block;
}

bool TranslateSingleThumbInstruction(IR::Block& block, LocationDescriptor descriptor, u32 thumb_instruction) {
    ThumbTranslatorVisitor visitor{block, descriptor, {}};

    const bool is_thumb_16 = IsThumb16(static_cast<u16>(thumb_instruction));
    bool should_continue = true;
    if (is_thumb_16) {
        if (const auto decoder = DecodeThumb16<ThumbTranslatorVisitor>(static_cast<u16>(thumb_instruction))) {
            should_continue = decoder->get().call(visitor, static_cast<u16>(thumb_instruction));
        } else {
            should_continue = visitor.thumb16_UDF();
        }
    } else {
        thumb_instruction = Common::SwapHalves32(thumb_instruction);
        if (const auto decoder = DecodeThumb32<ThumbTranslatorVisitor>(thumb_instruction)) {
            should_continue = decoder->get().call(visitor, thumb_instruction);
        } else {
            should_continue = visitor.thumb32_UDF();
        }
    }

    const s32 advance_pc = is_thumb_16 ? 2 : 4;
    visitor.ir.current_location = visitor.ir.current_location.AdvancePC(advance_pc);
    block.CycleCount()++;

    block.SetEndLocation(visitor.ir.current_location);

    return should_continue;
}

bool ThumbTranslatorVisitor::InterpretThisInstruction() {
    ir.SetTerm(IR::Term::Interpret(ir.current_location));
    return false;
}

bool ThumbTranslatorVisitor::UnpredictableInstruction() {
    ir.ExceptionRaised(Exception::UnpredictableInstruction);
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

bool ThumbTranslatorVisitor::UndefinedInstruction() {
    ir.ExceptionRaised(Exception::UndefinedInstruction);
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

bool ThumbTranslatorVisitor::RaiseException(Exception exception) {
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + 2)); // TODO: T32
    ir.ExceptionRaised(exception);
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

} // namespace Dynarmic::A32
