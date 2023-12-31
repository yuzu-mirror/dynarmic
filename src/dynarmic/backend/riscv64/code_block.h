/* This file is part of the dynarmic project.
 * Copyright (c) 2024 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstdint>
#include <new>

#include <biscuit/assembler.hpp>
#include <sys/mman.h>

namespace Dynarmic::Backend::RV64 {

class CodeBlock {
public:
    explicit CodeBlock(std::size_t size)
            : memsize(size) {
        mem = (std::uint32_t*)mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);

        if (mem == nullptr)
            throw std::bad_alloc{};
    }

    ~CodeBlock() {
        if (mem == nullptr)
            return;

        munmap(mem, memsize);
    }

    std::uint32_t* ptr() const {
        return mem;
    }

protected:
    std::uint32_t* mem;
    std::size_t memsize = 0;
    biscuit::Assembler as;
};
}  // namespace Dynarmic::Backend::RV64
