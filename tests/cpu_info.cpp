#include <array>
#include <utility>

#include <catch.hpp>
#include <fmt/format.h>
#include <xbyak_util.h>

TEST_CASE("Host CPU supports", "[a64]") {
    Xbyak::util::Cpu cpu_info;
    static constexpr std::array types{
        std::make_pair(Xbyak::util::Cpu::tMMX, "MMX"),
        std::make_pair(Xbyak::util::Cpu::tMMX2, "MMX2"),
        std::make_pair(Xbyak::util::Cpu::tCMOV, "CMOV"),
        std::make_pair(Xbyak::util::Cpu::tSSE, "SSE"),
        std::make_pair(Xbyak::util::Cpu::tSSE2, "SSE2"),
        std::make_pair(Xbyak::util::Cpu::tSSE3, "SSE3"),
        std::make_pair(Xbyak::util::Cpu::tSSSE3, "SSSE3"),
        std::make_pair(Xbyak::util::Cpu::tSSE41, "SSE41"),
        std::make_pair(Xbyak::util::Cpu::tSSE42, "SSE42"),
        std::make_pair(Xbyak::util::Cpu::tPOPCNT, "POPCNT"),
        std::make_pair(Xbyak::util::Cpu::tAESNI, "AESNI"),
        std::make_pair(Xbyak::util::Cpu::tSSE5, "SSE5"),
        std::make_pair(Xbyak::util::Cpu::tOSXSAVE, "OSXSAVE"),
        std::make_pair(Xbyak::util::Cpu::tPCLMULQDQ, "PCLMULQDQ"),
        std::make_pair(Xbyak::util::Cpu::tAVX, "AVX"),
        std::make_pair(Xbyak::util::Cpu::tFMA, "FMA"),
        std::make_pair(Xbyak::util::Cpu::t3DN, "3DN"),
        std::make_pair(Xbyak::util::Cpu::tE3DN, "E3DN"),
        std::make_pair(Xbyak::util::Cpu::tSSE4a, "SSE4a"),
        std::make_pair(Xbyak::util::Cpu::tRDTSCP, "RDTSCP"),
        std::make_pair(Xbyak::util::Cpu::tAVX2, "AVX2"),
        std::make_pair(Xbyak::util::Cpu::tBMI1, "BMI1"),
        std::make_pair(Xbyak::util::Cpu::tBMI2, "BMI2"),
        std::make_pair(Xbyak::util::Cpu::tLZCNT, "LZCNT"),
        std::make_pair(Xbyak::util::Cpu::tINTEL, "INTEL"),
        std::make_pair(Xbyak::util::Cpu::tAMD, "AMD"),
        std::make_pair(Xbyak::util::Cpu::tENHANCED_REP, "ENHANCED_REP"),
        std::make_pair(Xbyak::util::Cpu::tRDRAND, "RDRAND"),
        std::make_pair(Xbyak::util::Cpu::tADX, "ADX"),
        std::make_pair(Xbyak::util::Cpu::tRDSEED, "RDSEED"),
        std::make_pair(Xbyak::util::Cpu::tSMAP, "SMAP"),
        std::make_pair(Xbyak::util::Cpu::tHLE, "HLE"),
        std::make_pair(Xbyak::util::Cpu::tRTM, "RTM"),
        std::make_pair(Xbyak::util::Cpu::tF16C, "F16C"),
        std::make_pair(Xbyak::util::Cpu::tMOVBE, "MOVBE"),
        std::make_pair(Xbyak::util::Cpu::tAVX512F, "AVX512F"),
        std::make_pair(Xbyak::util::Cpu::tAVX512DQ, "AVX512DQ"),
        std::make_pair(Xbyak::util::Cpu::tAVX512_IFMA, "AVX512_IFMA"),
        std::make_pair(Xbyak::util::Cpu::tAVX512IFMA, "AVX512IFMA"),
        std::make_pair(Xbyak::util::Cpu::tAVX512PF, "AVX512PF"),
        std::make_pair(Xbyak::util::Cpu::tAVX512ER, "AVX512ER"),
        std::make_pair(Xbyak::util::Cpu::tAVX512CD, "AVX512CD"),
        std::make_pair(Xbyak::util::Cpu::tAVX512BW, "AVX512BW"),
        std::make_pair(Xbyak::util::Cpu::tAVX512VL, "AVX512VL"),
        std::make_pair(Xbyak::util::Cpu::tAVX512_VBMI, "AVX512_VBMI"),
        std::make_pair(Xbyak::util::Cpu::tAVX512VBMI, "AVX512VBMI"),
        std::make_pair(Xbyak::util::Cpu::tAVX512_4VNNIW, "AVX512_4VNNIW"),
        std::make_pair(Xbyak::util::Cpu::tAVX512_4FMAPS, "AVX512_4FMAPS"),
        std::make_pair(Xbyak::util::Cpu::tPREFETCHWT1, "PREFETCHWT1"),
        std::make_pair(Xbyak::util::Cpu::tPREFETCHW, "PREFETCHW"),
        std::make_pair(Xbyak::util::Cpu::tSHA, "SHA"),
        std::make_pair(Xbyak::util::Cpu::tMPX, "MPX"),
        std::make_pair(Xbyak::util::Cpu::tAVX512_VBMI2, "AVX512_VBMI2"),
        std::make_pair(Xbyak::util::Cpu::tGFNI, "GFNI"),
        std::make_pair(Xbyak::util::Cpu::tVAES, "VAES"),
        std::make_pair(Xbyak::util::Cpu::tVPCLMULQDQ, "VPCLMULQDQ"),
        std::make_pair(Xbyak::util::Cpu::tAVX512_VNNI, "AVX512_VNNI"),
        std::make_pair(Xbyak::util::Cpu::tAVX512_BITALG, "AVX512_BITALG"),
        std::make_pair(Xbyak::util::Cpu::tAVX512_VPOPCNTDQ, "AVX512_VPOPCNTDQ"),
    };

    for (const auto& [type, name] : types) {
        if (cpu_info.has(type)) {
            fmt::print("CPU has {}\n", name);
        }
    }
}
