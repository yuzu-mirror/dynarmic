/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <memory>
#include <mutex>

#include <boost/icl/interval_set.hpp>
#include <mcl/assert.hpp>
#include <mcl/scope_exit.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/atomic.h"
#include "dynarmic/interface/A64/a64.h"
#include "dynarmic/interface/A64/config.h"

namespace Dynarmic::A64 {

struct Jit::Impl {};

Jit::Jit(UserConfig conf) {
    (void)conf;
}

Jit::~Jit() = default;

HaltReason Jit::Run() {
    ASSERT_FALSE("not implemented");
}

HaltReason Jit::Step() {
    ASSERT_FALSE("not implemented");
}

void Jit::ClearCache() {
}

void Jit::InvalidateCacheRange(std::uint64_t start_address, std::size_t length) {
    (void)start_address;
    (void)length;
}

void Jit::Reset() {
}

void Jit::HaltExecution(HaltReason hr) {
    (void)hr;
}

void Jit::ClearHalt(HaltReason hr) {
    (void)hr;
}

std::uint64_t Jit::GetSP() const {
    return 0;
}

void Jit::SetSP(std::uint64_t value) {
    (void)value;
}

std::uint64_t Jit::GetPC() const {
    return 0;
}

void Jit::SetPC(std::uint64_t value) {
    (void)value;
}

std::uint64_t Jit::GetRegister(std::size_t index) const {
    (void)index;
    return 0;
}

void Jit::SetRegister(size_t index, std::uint64_t value) {
    (void)index;
    (void)value;
}

std::array<std::uint64_t, 31> Jit::GetRegisters() const {
    return {};
}

void Jit::SetRegisters(const std::array<std::uint64_t, 31>& value) {
    (void)value;
}

Vector Jit::GetVector(std::size_t index) const {
    (void)index;
    return {};
}

void Jit::SetVector(std::size_t index, Vector value) {
    (void)index;
    (void)value;
}

std::array<Vector, 32> Jit::GetVectors() const {
    return {};
}

void Jit::SetVectors(const std::array<Vector, 32>& value) {
    (void)value;
}

std::uint32_t Jit::GetFpcr() const {
    return 0;
}

void Jit::SetFpcr(std::uint32_t value) {
    (void)value;
}

std::uint32_t Jit::GetFpsr() const {
    return 0;
}

void Jit::SetFpsr(std::uint32_t value) {
    (void)value;
}

std::uint32_t Jit::GetPstate() const {
    return 0;
}

void Jit::SetPstate(std::uint32_t value) {
    (void)value;
}

void Jit::ClearExclusiveState() {
}

bool Jit::IsExecuting() const {
    return false;
}

void Jit::DumpDisassembly() const {
    ASSERT_FALSE("not implemented");
}

std::vector<std::string> Jit::Disassemble() const {
    ASSERT_FALSE("not implemented");
}

}  // namespace Dynarmic::A64
