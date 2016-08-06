/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <memory>

#include "common/common_types.h"

namespace Dynarmic {

namespace Arm {
struct LocationDescriptor;
}

class Jit;

/// These function pointers may be inserted into compiled code.
struct UserCallbacks {
    u8 (*MemoryRead8)(u32 vaddr);
    u16 (*MemoryRead16)(u32 vaddr);
    u32 (*MemoryRead32)(u32 vaddr);
    u64 (*MemoryRead64)(u32 vaddr);

    void (*MemoryWrite8)(u32 vaddr, u8 value);
    void (*MemoryWrite16)(u32 vaddr, u16 value);
    void (*MemoryWrite32)(u32 vaddr, u32 value);
    void (*MemoryWrite64)(u32 vaddr, u64 value);

    bool (*IsReadOnlyMemory)(u32 vaddr);

    void (*InterpreterFallback)(u32 pc, Jit* jit);

    bool (*CallSVC)(u32 swi);
};

class Jit final {
public:
    explicit Jit(Dynarmic::UserCallbacks callbacks);
    ~Jit();

    /**
     * Runs the emulated CPU for about cycle_count cycles.
     * Cannot be recursively called.
     * @param cycle_count Estimated number of cycles to run the CPU for.
     * @returns Actual cycle count.
     */
    size_t Run(size_t cycle_count);

    /**
     * Clears the code cache of all compiled code.
     * Cannot be called from a callback.
     * @param poison_memory If true, poisons memory to crash if any stray code pointers are called.
     */
    void ClearCache(bool poison_memory = true);

    /**
     * Stops execution in Jit::Run.
     * Can only be called from a callback.
     */
    void HaltExecution();

    /// View and modify registers.
    std::array<u32, 16>& Regs();
    std::array<u32, 16> Regs() const;
    std::array<u32, 64>& ExtRegs();
    std::array<u32, 64> ExtRegs() const;

    /// View and modify CPSR.
    u32& Cpsr();
    u32 Cpsr() const;

    /// View and modify FPSCR.
    u32 Fpscr() const;
    void SetFpscr(u32 value) const;

    /**
     * Returns true if Jit::Run was called but hasn't returned yet.
     * i.e.: We're in a callback.
     */
    bool IsExecuting() const {
        return is_executing;
    }

    std::string Disassemble(const Arm::LocationDescriptor& descriptor);

private:
    bool halt_requested = false;
    bool is_executing = false;

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Dynarmic
