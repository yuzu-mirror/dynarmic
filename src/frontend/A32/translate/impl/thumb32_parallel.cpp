/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "frontend/A32/translate/impl/translate_thumb.h"

namespace Dynarmic::A32 {

bool ThumbTranslatorVisitor::thumb32_SADD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddS8(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddS16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SASX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddSubS16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SSAX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubAddS16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SSUB8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubS8(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_SSUB16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubS16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UADD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddU8(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddU16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UASX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddSubU16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_USAX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubAddU16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_USUB8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubU8(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_USUB16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubU16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_QADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedAddS16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool ThumbTranslatorVisitor::thumb32_UQADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedAddU16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

} // namespace Dynarmic::A32
