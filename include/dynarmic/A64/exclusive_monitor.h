/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Dynarmic {
namespace A64 {

using VAddr = std::uint64_t;

class ExclusiveMonitor {
public:
    /// @param processor_count Maximum number of processors using this global
    ///                        exclusive monitor. Each processor must have a
    ///                        unique id.
    explicit ExclusiveMonitor(size_t processor_count);

    size_t GetProcessorCount() const;

    /// Marks a region containing [address, address+size) to be exclusive to
    /// processor processor_id.
    void Mark(size_t processor_id, VAddr address, size_t size);

    /// Checks to see if processor processor_id has exclusive access to the
    /// specified region. If it does, executes the operation then clears
    /// the exclusive state for processors if their exclusive region(s)
    /// contain [address, address+size).
    template <typename Function>
    bool DoExclusiveOperation(size_t processor_id, VAddr address, size_t size, Function op) {
        if (!CheckAndClear(processor_id, address, size)) {
            return false;
        }

        op();

        Unlock();
        return true;
    }

    /// Unmark everything.
    void Clear();
    /// Unmark processor id
    void Clear(size_t processor_id);

private:
    bool CheckAndClear(size_t processor_id, VAddr address, size_t size);

    void Lock();
    void Unlock();

    static constexpr VAddr RESERVATION_GRANULE_MASK = 0xFFFF'FFFF'FFFF'FFFFull;
    static constexpr VAddr INVALID_EXCLUSIVE_ADDRESS = 0xDEAD'DEAD'DEAD'DEADull;
    std::atomic_flag is_locked;
    std::vector<VAddr> exclusive_addresses;
};

} // namespace A64
} // namespace Dynarmic
