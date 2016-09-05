/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Dynarmic {

class Jit;

/// These function pointers may be inserted into compiled code.
struct UserCallbacks {
    std::uint8_t (*MemoryRead8)(std::uint32_t vaddr);
    std::uint16_t (*MemoryRead16)(std::uint32_t vaddr);
    std::uint32_t (*MemoryRead32)(std::uint32_t vaddr);
    std::uint64_t (*MemoryRead64)(std::uint32_t vaddr);

    void (*MemoryWrite8)(std::uint32_t vaddr, std::uint8_t value);
    void (*MemoryWrite16)(std::uint32_t vaddr, std::uint16_t value);
    void (*MemoryWrite32)(std::uint32_t vaddr, std::uint32_t value);
    void (*MemoryWrite64)(std::uint32_t vaddr, std::uint64_t value);

    bool (*IsReadOnlyMemory)(std::uint32_t vaddr);

    /// The intrepreter must execute only one instruction at PC.
    void (*InterpreterFallback)(std::uint32_t pc, Jit* jit, void* user_arg);
    void* user_arg = nullptr;

    void (*CallSVC)(std::uint32_t swi);

    // Page Table
    static constexpr std::size_t PAGE_BITS = 12;
    static constexpr std::size_t NUM_PAGE_TABLE_ENTRIES = 1 << (32 - PAGE_BITS);
    std::array<std::uint8_t*, NUM_PAGE_TABLE_ENTRIES>* page_table = nullptr;
};

} // namespace Dynarmic
