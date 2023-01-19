/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <algorithm>
#include <array>
#include <functional>

#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/frontend/A32/a32_ir_emitter.h"
#include "dynarmic/frontend/A32/a32_types.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/opcodes.h"
#include "dynarmic/ir/opt/passes.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::Optimization {

void A32GetSetElimination(IR::Block& block, A32GetSetEliminationOptions) {
    using Iterator = std::reverse_iterator<IR::Block::iterator>;

    struct RegisterInfo {
        bool set_not_required = false;
        bool has_value_request = false;
        Iterator value_request = {};
    };
    struct ValuelessRegisterInfo {
        bool set_not_required = false;
    };
    std::array<RegisterInfo, 15> reg_info;
    std::array<RegisterInfo, 64> ext_reg_singles_info;
    std::array<RegisterInfo, 32> ext_reg_doubles_info;
    std::array<RegisterInfo, 32> ext_reg_vector_double_info;
    std::array<RegisterInfo, 16> ext_reg_vector_quad_info;
    ValuelessRegisterInfo nzcvq;
    ValuelessRegisterInfo nzcv;
    ValuelessRegisterInfo nz;
    RegisterInfo c_flag;
    RegisterInfo ge;

    auto do_set = [&](RegisterInfo& info, IR::Value value, Iterator inst, std::initializer_list<std::reference_wrapper<RegisterInfo>> dependants = {}) {
        if (info.has_value_request) {
            info.value_request->ReplaceUsesWith(value);
        }
        info.has_value_request = false;

        if (info.set_not_required && std::all_of(dependants.begin(), dependants.end(), [](auto d) { return !d.get().has_value_request; })) {
            inst->Invalidate();
        }
        info.set_not_required = true;

        for (auto d : dependants) {
            d.get() = {};
        }
    };

    auto do_set_valueless = [&](ValuelessRegisterInfo& info, Iterator inst) {
        if (info.set_not_required) {
            inst->Invalidate();
        }
        info.set_not_required = true;
    };

    auto do_get = [](RegisterInfo& info, Iterator inst) {
        if (info.has_value_request) {
            info.value_request->ReplaceUsesWith(IR::Value{&*inst});
        }
        info.has_value_request = true;
        info.value_request = inst;
    };

    A32::IREmitter ir{block, A32::LocationDescriptor{block.Location()}, {}};

    for (auto inst = block.rbegin(); inst != block.rend(); ++inst) {
        switch (inst->GetOpcode()) {
        case IR::Opcode::A32SetRegister: {
            const A32::Reg reg = inst->GetArg(0).GetA32RegRef();
            if (reg == A32::Reg::PC) {
                break;
            }
            const auto reg_index = static_cast<size_t>(reg);
            do_set(reg_info[reg_index], inst->GetArg(1), inst);
            break;
        }
        case IR::Opcode::A32GetRegister: {
            const A32::Reg reg = inst->GetArg(0).GetA32RegRef();
            ASSERT(reg != A32::Reg::PC);
            const size_t reg_index = static_cast<size_t>(reg);
            do_get(reg_info[reg_index], inst);
            break;
        }
        case IR::Opcode::A32SetExtendedRegister32: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            do_set(ext_reg_singles_info[reg_index],
                   inst->GetArg(1),
                   inst,
                   {
                       ext_reg_doubles_info[reg_index / 2],
                       ext_reg_vector_double_info[reg_index / 2],
                       ext_reg_vector_quad_info[reg_index / 4],
                   });
            break;
        }
        case IR::Opcode::A32GetExtendedRegister32: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            do_get(ext_reg_singles_info[reg_index], inst);
            break;
        }
        case IR::Opcode::A32SetExtendedRegister64: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            do_set(ext_reg_doubles_info[reg_index],
                   inst->GetArg(1),
                   inst,
                   {
                       ext_reg_singles_info[reg_index * 2 + 0],
                       ext_reg_singles_info[reg_index * 2 + 1],
                       ext_reg_vector_double_info[reg_index],
                       ext_reg_vector_quad_info[reg_index / 2],
                   });
            break;
        }
        case IR::Opcode::A32GetExtendedRegister64: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            do_get(ext_reg_doubles_info[reg_index], inst);
            break;
        }
        case IR::Opcode::A32SetVector: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            if (A32::IsDoubleExtReg(reg)) {
                do_set(ext_reg_vector_double_info[reg_index],
                       inst->GetArg(1),
                       inst,
                       {
                           ext_reg_singles_info[reg_index * 2 + 0],
                           ext_reg_singles_info[reg_index * 2 + 1],
                           ext_reg_doubles_info[reg_index],
                           ext_reg_vector_quad_info[reg_index / 2],
                       });
            } else {
                DEBUG_ASSERT(A32::IsQuadExtReg(reg));

                do_set(ext_reg_vector_quad_info[reg_index],
                       inst->GetArg(1),
                       inst,
                       {
                           ext_reg_singles_info[reg_index * 4 + 0],
                           ext_reg_singles_info[reg_index * 4 + 1],
                           ext_reg_singles_info[reg_index * 4 + 2],
                           ext_reg_singles_info[reg_index * 4 + 3],
                           ext_reg_doubles_info[reg_index * 2 + 0],
                           ext_reg_doubles_info[reg_index * 2 + 1],
                           ext_reg_vector_double_info[reg_index * 2 + 0],
                           ext_reg_vector_double_info[reg_index * 2 + 1],
                       });
            }
            break;
        }
        case IR::Opcode::A32GetVector: {
            const A32::ExtReg reg = inst->GetArg(0).GetA32ExtRegRef();
            const size_t reg_index = A32::RegNumber(reg);
            if (A32::IsDoubleExtReg(reg)) {
                do_get(ext_reg_vector_double_info[reg_index], inst);
            } else {
                DEBUG_ASSERT(A32::IsQuadExtReg(reg));
                do_get(ext_reg_vector_quad_info[reg_index], inst);
            }
            break;
        }
        case IR::Opcode::A32GetCFlag: {
            do_get(c_flag, inst);
            break;
        }
        case IR::Opcode::A32SetCpsrNZCV: {
            if (c_flag.has_value_request) {
                ir.SetInsertionPointBefore(inst.base());  // base is one ahead
                IR::U1 c = ir.GetCFlagFromNZCV(IR::NZCV{inst->GetArg(0)});
                c_flag.value_request->ReplaceUsesWith(c);
                c_flag.has_value_request = false;
                break;  // This case will be executed again because of the above
            }

            do_set_valueless(nzcv, inst);

            nz = {.set_not_required = true};
            c_flag = {.set_not_required = true};
            break;
        }
        case IR::Opcode::A32SetCpsrNZCVRaw: {
            if (c_flag.has_value_request) {
                nzcv.set_not_required = false;
            }

            do_set_valueless(nzcv, inst);

            nzcvq = {};
            nz = {.set_not_required = true};
            c_flag = {.set_not_required = true};
            break;
        }
        case IR::Opcode::A32SetCpsrNZCVQ: {
            if (c_flag.has_value_request) {
                nzcvq.set_not_required = false;
            }

            do_set_valueless(nzcvq, inst);

            nzcv = {.set_not_required = true};
            nz = {.set_not_required = true};
            c_flag = {.set_not_required = true};
            break;
        }
        case IR::Opcode::A32SetCpsrNZ: {
            do_set_valueless(nz, inst);

            nzcvq = {};
            nzcv = {};
            break;
        }
        case IR::Opcode::A32SetCpsrNZC: {
            if (c_flag.has_value_request) {
                c_flag.value_request->ReplaceUsesWith(inst->GetArg(1));
                c_flag.has_value_request = false;
            }

            if (!inst->GetArg(1).IsImmediate() && inst->GetArg(1).GetInstRecursive()->GetOpcode() == IR::Opcode::A32GetCFlag) {
                const auto nz_value = inst->GetArg(0);

                inst->Invalidate();

                ir.SetInsertionPointBefore(inst.base());
                ir.SetCpsrNZ(IR::NZCV{nz_value});

                nzcvq = {};
                nzcv = {};
                nz = {.set_not_required = true};
                break;
            }

            if (nz.set_not_required && c_flag.set_not_required) {
                inst->Invalidate();
            } else if (nz.set_not_required) {
                inst->SetArg(0, IR::Value::EmptyNZCVImmediateMarker());
            }
            nz.set_not_required = true;
            c_flag.set_not_required = true;

            nzcv = {};
            nzcvq = {};
            break;
        }
        case IR::Opcode::A32SetGEFlags: {
            do_set(ge, inst->GetArg(0), inst);
            break;
        }
        case IR::Opcode::A32GetGEFlags: {
            do_get(ge, inst);
            break;
        }
        case IR::Opcode::A32SetGEFlagsCompressed: {
            ge = {.set_not_required = true};
            break;
        }
        case IR::Opcode::A32OrQFlag: {
            break;
        }
        default: {
            if (inst->ReadsFromCPSR() || inst->WritesToCPSR()) {
                nzcvq = {};
                nzcv = {};
                nz = {};
                c_flag = {};
                ge = {};
            }
            if (inst->ReadsFromCoreRegister() || inst->WritesToCoreRegister()) {
                reg_info = {};
                ext_reg_singles_info = {};
                ext_reg_doubles_info = {};
                ext_reg_vector_double_info = {};
                ext_reg_vector_quad_info = {};
            }
            break;
        }
        }
    }
}

}  // namespace Dynarmic::Optimization
