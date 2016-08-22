/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <array>

#include "common/assert.h"
#include "common/common_types.h"
#include "frontend/ir/basic_block.h"
#include "frontend/ir/value.h"
#include "ir_opt/passes.h"

namespace Dynarmic {
namespace Optimization {

void GetSetElimination(IR::Block& block) {
    using Iterator = decltype(block.begin());
    struct RegisterInfo {
        IR::Value register_value;
        bool set_instruction_present = false;
        Iterator last_set_instruction;
    };
    std::array<RegisterInfo, 15> reg_info;
    RegisterInfo n_info;
    RegisterInfo z_info;
    RegisterInfo c_info;
    RegisterInfo v_info;

    const auto do_set = [&block](RegisterInfo& info, IR::Value value, Iterator set_inst) {
        if (info.set_instruction_present) {
            info.last_set_instruction->Invalidate();
            block.instructions.erase(info.last_set_instruction);
        }

        info.register_value = value;
        info.set_instruction_present = true;
        info.last_set_instruction = set_inst;
    };

    const auto do_get = [](RegisterInfo& info, Iterator get_inst) {
        if (info.register_value.IsEmpty()) {
            info.register_value = IR::Value(&*get_inst);
            return;
        }
        get_inst->ReplaceUsesWith(info.register_value);
    };

    for (auto inst = block.begin(); inst != block.end(); ++inst) {
        switch (inst->GetOpcode()) {
        case IR::Opcode::SetRegister: {
            Arm::Reg reg = inst->GetArg(0).GetRegRef();
            if (reg == Arm::Reg::PC)
                break;
            size_t reg_index = static_cast<size_t>(reg);
            do_set(reg_info[reg_index], inst->GetArg(1), inst);
            break;
        }
        case IR::Opcode::GetRegister: {
            Arm::Reg reg = inst->GetArg(0).GetRegRef();
            ASSERT(reg != Arm::Reg::PC);
            size_t reg_index = static_cast<size_t>(reg);
            do_get(reg_info[reg_index], inst);
            break;
        }
        case IR::Opcode::SetNFlag: {
            do_set(n_info, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::GetNFlag: {
            do_get(n_info, inst);
            break;
        }
        case IR::Opcode::SetZFlag: {
            do_set(z_info, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::GetZFlag: {
            do_get(z_info, inst);
            break;
        }
        case IR::Opcode::SetCFlag: {
            do_set(c_info, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::GetCFlag: {
            do_get(c_info, inst);
            break;
        }
        case IR::Opcode::SetVFlag: {
            do_set(v_info, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::GetVFlag: {
            do_get(v_info, inst);
            break;
        }
        case IR::Opcode::SetCpsr:
        case IR::Opcode::GetCpsr: {
            n_info = {};
            z_info = {};
            c_info = {};
            v_info = {};
            break;
        }
        default:
            break;
        }
    }
}

} // namespace Optimization
} // namespace Dynarmic
