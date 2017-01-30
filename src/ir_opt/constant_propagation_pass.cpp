/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <dynarmic/callbacks.h>

#include "frontend/ir/basic_block.h"
#include "frontend/ir/opcodes.h"
#include "ir_opt/passes.h"

namespace Dynarmic {
namespace Optimization {

void ConstantPropagation(IR::Block& block, const UserCallbacks::Memory& memory_callbacks) {
    for (auto& inst : block) {
        if (!inst.AreAllArgsImmediates())
            continue;

        switch (inst.GetOpcode()) {
        case IR::Opcode::ReadMemory8: {
            u32 vaddr = inst.GetArg(0).GetU32();
            if (memory_callbacks.IsReadOnlyMemory(vaddr)) {
                u8 value_from_memory = memory_callbacks.Read8(vaddr);
                inst.ReplaceUsesWith(IR::Value{value_from_memory});
            }
            break;
        }
        case IR::Opcode::ReadMemory16: {
            u32 vaddr = inst.GetArg(0).GetU32();
            if (memory_callbacks.IsReadOnlyMemory(vaddr)) {
                u16 value_from_memory = memory_callbacks.Read16(vaddr);
                inst.ReplaceUsesWith(IR::Value{value_from_memory});
            }
            break;
        }
        case IR::Opcode::ReadMemory32: {
            u32 vaddr = inst.GetArg(0).GetU32();
            if (memory_callbacks.IsReadOnlyMemory(vaddr)) {
                u32 value_from_memory = memory_callbacks.Read32(vaddr);
                inst.ReplaceUsesWith(IR::Value{value_from_memory});
            }
            break;
        }
        case IR::Opcode::ReadMemory64: {
            u32 vaddr = inst.GetArg(0).GetU32();
            if (memory_callbacks.IsReadOnlyMemory(vaddr)) {
                u64 value_from_memory = memory_callbacks.Read64(vaddr);
                inst.ReplaceUsesWith(IR::Value{value_from_memory});
            }
            break;
        }
        case IR::Opcode::ZeroExtendByteToWord: {
            u8 byte = inst.GetArg(0).GetU8();
            u32 value = static_cast<u32>(byte);
            inst.ReplaceUsesWith(IR::Value{value});
            break;
        }
        case IR::Opcode::ZeroExtendHalfToWord: {
            u16 half = inst.GetArg(0).GetU16();
            u32 value = static_cast<u32>(half);
            inst.ReplaceUsesWith(IR::Value{value});
            break;
        }
        default:
            break;
        }
    }
}

} // namespace Optimization
} // namespace Dynarmic
