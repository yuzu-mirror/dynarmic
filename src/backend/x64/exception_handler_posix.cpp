/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "backend/x64/exception_handler.h"

#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <signal.h>
#ifdef __APPLE__
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif

#include "backend/x64/block_of_code.h"
#include "common/assert.h"
#include "common/cast_util.h"
#include "common/common_types.h"

namespace Dynarmic::Backend::X64 {

namespace {

struct CodeBlockInfo {
    u64 code_begin, code_end;
    std::function<FakeCall(u64)> cb;
};

class SigHandler {
public:
    SigHandler();

    void AddCodeBlock(CodeBlockInfo info);
    void RemoveCodeBlock(u64 rip);

private:
    auto FindCodeBlockInfo(u64 rip) {
        return std::find_if(code_block_infos.begin(), code_block_infos.end(), [&](const auto& x) { return x.code_begin <= rip && x.code_end > rip; });
    }

    std::vector<CodeBlockInfo> code_block_infos;
    std::mutex code_block_infos_mutex;

    struct sigaction old_sa_segv;
    struct sigaction old_sa_bus;

    static void SigAction(int sig, siginfo_t* info, void* raw_context);
};

SigHandler sig_handler;

SigHandler::SigHandler() {
    constexpr size_t signal_stack_size = std::max(SIGSTKSZ, 2 * 1024 * 1024);

    stack_t signal_stack;
    signal_stack.ss_sp = std::malloc(signal_stack_size);
    signal_stack.ss_size = signal_stack_size;
    signal_stack.ss_flags = 0;
    const int ret = sigaltstack(&signal_stack, nullptr);
    ASSERT_MSG(ret == 0, "dynarmic: POSIX SigHandler: init failure at sigaltstack");

    struct sigaction sa;
    sa.sa_handler = nullptr;
    sa.sa_sigaction = &SigHandler::SigAction;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &old_sa_segv);
#ifdef __APPLE__
    sigaction(SIGBUS, &sa, &old_sa_bus);
#endif
}

void SigHandler::AddCodeBlock(CodeBlockInfo cbi) {
    std::lock_guard<std::mutex> guard(code_block_infos_mutex);
    if (auto iter = FindCodeBlockInfo(cbi.code_begin); iter != code_block_infos.end()) {
        code_block_infos.erase(iter);
    }
    code_block_infos.push_back(cbi);
}

void SigHandler::RemoveCodeBlock(u64 rip) {
    std::lock_guard<std::mutex> guard(code_block_infos_mutex);
    const auto iter = FindCodeBlockInfo(rip);
    if (iter == code_block_infos.end()) {
        return;
    }
    code_block_infos.erase(iter);
}

void SigHandler::SigAction(int sig, siginfo_t* info, void* raw_context) {
    ASSERT(sig == SIGSEGV || sig == SIGBUS);

#if defined(__APPLE__)
    #define CTX_RIP (((ucontext_t*)raw_context)->uc_mcontext->__ss.__rip)
    #define CTX_RSP (((ucontext_t*)raw_context)->uc_mcontext->__ss.__rsp)
#elif defined(__linux__)
    #define CTX_RIP (((ucontext_t*)raw_context)->uc_mcontext.gregs[REG_RIP])
    #define CTX_RSP (((ucontext_t*)raw_context)->uc_mcontext.gregs[REG_RSP])
#elif defined(__FreeBSD__)
    #define CTX_RIP (((ucontext_t*)raw_context)->uc_mcontext.mc_rip)
    #define CTX_RSP (((ucontext_t*)raw_context)->uc_mcontext.mc_rsp)
#else
    #error "Unknown platform"
#endif

    {
        std::lock_guard<std::mutex> guard(sig_handler.code_block_infos_mutex);

        const auto iter = sig_handler.FindCodeBlockInfo(CTX_RIP);
        if (iter != sig_handler.code_block_infos.end()) {
            FakeCall fc = iter->cb(CTX_RIP);

            CTX_RSP -= sizeof(u64);
            *Common::BitCast<u64*>(CTX_RSP) = fc.ret_rip;
            CTX_RIP = fc.call_rip;

            return;
        }
    }

    fmt::print(stderr, "dynarmic: POSIX SigHandler: Exception was not in registered code blocks (rip {:#016x})\n", CTX_RIP);

    struct sigaction* retry_sa = sig == SIGSEGV ? &sig_handler.old_sa_segv : &sig_handler.old_sa_bus;
    if (retry_sa->sa_flags & SA_SIGINFO) {
      retry_sa->sa_sigaction(sig, info, raw_context);
      return;
    }
    if (retry_sa->sa_handler == SIG_DFL) {
      signal(sig, SIG_DFL);
      return;
    }
    if (retry_sa->sa_handler == SIG_IGN) {
      return;
    }
    retry_sa->sa_handler(sig);
}

} // anonymous namespace

struct ExceptionHandler::Impl final {
    Impl(BlockOfCode& code)
        : code_begin(Common::BitCast<u64>(code.getCode()))
        , code_end(code_begin + code.GetTotalCodeSize())
    {}

    void SetCallback(std::function<FakeCall(u64)> cb) {
        CodeBlockInfo cbi;
        cbi.code_begin = code_begin;
        cbi.code_end = code_end;
        cbi.cb = cb;
        sig_handler.AddCodeBlock(cbi);
    }

    ~Impl() {
        sig_handler.RemoveCodeBlock(code_begin);
    }

private:
    u64 code_begin, code_end;
};

ExceptionHandler::ExceptionHandler() = default;
ExceptionHandler::~ExceptionHandler() = default;

void ExceptionHandler::Register(BlockOfCode& code) {
    impl = std::make_unique<Impl>(code);
}

bool ExceptionHandler::SupportsFastmem() const noexcept {
    return static_cast<bool>(impl);
}

void ExceptionHandler::SetFastmemCallback(std::function<FakeCall(u64)> cb) {
    impl->SetCallback(cb);
}

} // namespace Dynarmic::Backend::X64
