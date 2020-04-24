/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace Dynarmic {
namespace A32 {

using VAddr = std::uint32_t;

class Coprocessor;

enum class Exception {
    /// An UndefinedFault occured due to executing instruction with an unallocated encoding
    UndefinedInstruction,
    /// An unpredictable instruction is to be executed. Implementation-defined behaviour should now happen.
    /// This behaviour is up to the user of this library to define.
    UnpredictableInstruction,
    /// A SEV instruction was executed. The event register of all PEs should be set. (Hint instruction.)
    SendEvent,
    /// A SEVL instruction was executed. The event register of the current PE should be set. (Hint instruction.)
    SendEventLocal,
    /// A WFI instruction was executed. You may now enter a low-power state. (Hint instruction.)
    WaitForInterrupt,
    /// A WFE instruction was executed. You may now enter a low-power state if the event register is clear. (Hint instruction.)
    WaitForEvent,
    /// A YIELD instruction was executed. (Hint instruction.)
    Yield,
    /// A BKPT instruction was executed.
    Breakpoint,
    /// A PLD instruction was executed. (Hint instruction.)
    PreloadData,
    /// A PLDW instruction was executed. (Hint instruction.)
    PreloadDataWithIntentToWrite,
};

/// These function pointers may be inserted into compiled code.
struct UserCallbacks {
    virtual ~UserCallbacks() = default;

    // All reads through this callback are 4-byte aligned.
    // Memory must be interpreted as little endian.
    virtual std::uint32_t MemoryReadCode(VAddr vaddr) { return MemoryRead32(vaddr); }

    // Reads through these callbacks may not be aligned.
    // Memory must be interpreted as if ENDIANSTATE == 0, endianness will be corrected by the JIT.
    virtual std::uint8_t MemoryRead8(VAddr vaddr) = 0;
    virtual std::uint16_t MemoryRead16(VAddr vaddr) = 0;
    virtual std::uint32_t MemoryRead32(VAddr vaddr) = 0;
    virtual std::uint64_t MemoryRead64(VAddr vaddr) = 0;

    // Writes through these callbacks may not be aligned.
    virtual void MemoryWrite8(VAddr vaddr, std::uint8_t value) = 0;
    virtual void MemoryWrite16(VAddr vaddr, std::uint16_t value) = 0;
    virtual void MemoryWrite32(VAddr vaddr, std::uint32_t value) = 0;
    virtual void MemoryWrite64(VAddr vaddr, std::uint64_t value) = 0;

    // If this callback returns true, the JIT will assume MemoryRead* callbacks will always
    // return the same value at any point in time for this vaddr. The JIT may use this information
    // in optimizations.
    // A conservative implementation that always returns false is safe.
    virtual bool IsReadOnlyMemory(VAddr /* vaddr */) { return false; }

    /// The interpreter must execute exactly num_instructions starting from PC.
    virtual void InterpreterFallback(VAddr pc, size_t num_instructions) = 0;

    // This callback is called whenever a SVC instruction is executed.
    virtual void CallSVC(std::uint32_t swi) = 0;

    virtual void ExceptionRaised(VAddr pc, Exception exception) = 0;

    // Timing-related callbacks
    // ticks ticks have passed
    virtual void AddTicks(std::uint64_t ticks) = 0;
    // How many more ticks am I allowed to execute?
    virtual std::uint64_t GetTicksRemaining() = 0;
};

struct UserConfig {
    UserCallbacks* callbacks;

    /// When set to false, this disables all optimizations than can't otherwise be disabled
    /// by setting other configuration options. This includes:
    /// - IR optimizations
    /// - Block linking optimizations
    /// - RSB optimizations
    /// This is intended to be used for debugging.
    bool enable_optimizations = true;

    // Page Table
    // The page table is used for faster memory access. If an entry in the table is nullptr,
    // the JIT will fallback to calling the MemoryRead*/MemoryWrite* callbacks.
    static constexpr std::size_t PAGE_BITS = 12;
    static constexpr std::size_t NUM_PAGE_TABLE_ENTRIES = 1 << (32 - PAGE_BITS);
    std::array<std::uint8_t*, NUM_PAGE_TABLE_ENTRIES>* page_table = nullptr;
    /// Determines if the pointer in the page_table shall be offseted locally or globally.
    /// 'false' will access page_table[addr >> bits][addr & mask]
    /// 'true'  will access page_table[addr >> bits][addr]
    /// Note: page_table[addr >> bits] will still be checked to verify active pages.
    ///       So there might be wrongly faulted pages which maps to nullptr.
    ///       This can be avoided by carefully allocating the memory region.
    bool absolute_offset_page_table = false;

    // Fastmem Pointer
    // This should point to the beginning of a 4GB address space which is in arranged just like
    // what you wish for emulated memory to be. If the host page faults on an address, the JIT
    // will fallback to calling the MemoryRead*/MemoryWrite* callbacks.
    void* fastmem_pointer = nullptr;
    /// Determines if instructions that pagefault should cause recompilation of that block
    /// with fastmem disabled.
    bool recompile_on_fastmem_failure = true;

    // Coprocessors
    std::array<std::shared_ptr<Coprocessor>, 16> coprocessors{};

    /// Hint instructions would cause ExceptionRaised to be called with the appropriate
    /// argument.
    bool hook_hint_instructions = false;

    /// This option relates to translation. Generally when we run into an unpredictable
    /// instruction the ExceptionRaised callback is called. If this is true, we define
    /// definite behaviour for some unpredictable instructions.
    bool define_unpredictable_behaviour = false;

    /// This enables the fast dispatcher.
    bool enable_fast_dispatch = true;

    /// This option relates to the CPSR.E flag. Enabling this option disables modification
    /// of CPSR.E by the emulated program, forcing it to 0.
    /// NOTE: Calling Jit::SetCpsr with CPSR.E=1 while this option is enabled may result
    ///       in unusual behavior.
    bool always_little_endian = false;
};

} // namespace A32
} // namespace Dynarmic
