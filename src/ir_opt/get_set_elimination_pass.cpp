/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "common/assert.h"
#include "frontend/ir/ir.h"
#include "ir_opt/passes.h"

namespace Dynarmic {
namespace Optimization {

void GetSetElimination(IR::Block& block) {
#if 0
    using Iterator = decltype(block.instructions.begin());
    struct RegisterInfo {
        IR::ValuePtr register_value = nullptr;
        bool set_instruction_present = false;
        Iterator last_set_instruction;
    };
    std::array<RegisterInfo, 15> reg_info;
    RegisterInfo n_info;
    RegisterInfo z_info;
    RegisterInfo c_info;
    RegisterInfo v_info;

    const auto do_set = [&block](RegisterInfo& info, IR::ValuePtr value, Iterator set_inst) {
        if (info.set_instruction_present) {
            (*info.last_set_instruction)->Invalidate();
            block.instructions.erase(info.last_set_instruction);
        }

        info.register_value = value;
        info.set_instruction_present = true;
        info.last_set_instruction = set_inst;
    };

    const auto do_get = [](RegisterInfo& info, Iterator get_inst) {
        if (!info.register_value) {
            info.register_value = *get_inst;
            return;
        }
        (*get_inst)->ReplaceUsesWith(info.register_value);
    };

    for (auto iter = block.instructions.begin(); iter != block.instructions.end(); ++iter) {
        switch ((*iter)->GetOpcode()) {
            case IR::Opcode::SetRegister: {
                auto inst = reinterpret_cast<IR::Inst*>((*iter).get());
                Arm::Reg reg = reinterpret_cast<IR::ImmRegRef*>(inst->GetArg(0).get())->value;
                if (reg == Arm::Reg::PC)
                    break;
                size_t reg_index = static_cast<size_t>(reg);
                do_set(reg_info[reg_index], inst->GetArg(1), iter);
                break;
            }
            case IR::Opcode::GetRegister: {
                auto inst = reinterpret_cast<IR::Inst*>((*iter).get());
                Arm::Reg reg = reinterpret_cast<IR::ImmRegRef*>(inst->GetArg(0).get())->value;
                ASSERT(reg != Arm::Reg::PC);
                size_t reg_index = static_cast<size_t>(reg);
                do_get(reg_info[reg_index], iter);
                break;
            }
            case IR::Opcode::SetNFlag: {
                auto inst = reinterpret_cast<IR::Inst*>((*iter).get());
                do_set(n_info, inst->GetArg(0), iter);
                break;
            }
            case IR::Opcode::GetNFlag: {
                do_get(n_info, iter);
                break;
            }
            case IR::Opcode::SetZFlag: {
                auto inst = reinterpret_cast<IR::Inst*>((*iter).get());
                do_set(z_info, inst->GetArg(0), iter);
                break;
            }
            case IR::Opcode::GetZFlag: {
                do_get(z_info, iter);
                break;
            }
            case IR::Opcode::SetCFlag: {
                auto inst = reinterpret_cast<IR::Inst*>((*iter).get());
                do_set(c_info, inst->GetArg(0), iter);
                break;
            }
            case IR::Opcode::GetCFlag: {
                do_get(c_info, iter);
                break;
            }
            case IR::Opcode::SetVFlag: {
                auto inst = reinterpret_cast<IR::Inst*>((*iter).get());
                do_set(v_info, inst->GetArg(0), iter);
                break;
            }
            case IR::Opcode::GetVFlag: {
                do_get(v_info, iter);
                break;
            }
            default:
                break;
        }
    }
#endif
}

} // namespace Optimization
} // namespace Dynarmic
