// Three registers of the same length
INST(asimd_VHADD,           "VHADD",                    "1111001U0Dzznnnndddd0000NQM0mmmm") // ASIMD
INST(asimd_VQADD,           "VQADD",                    "1111001U0Dzznnnndddd0000NQM1mmmm") // ASIMD
INST(asimd_VRHADD,          "VRHADD",                   "1111001U0Dzznnnndddd0001NQM0mmmm") // ASIMD
INST(asimd_VAND_reg,        "VAND (register)",          "111100100D00nnnndddd0001NQM1mmmm") // ASIMD
INST(asimd_VBIC_reg,        "VBIC (register)",          "111100100D01nnnndddd0001NQM1mmmm") // ASIMD
INST(asimd_VORR_reg,        "VORR (register)",          "111100100D10nnnndddd0001NQM1mmmm") // ASIMD
INST(asimd_VORN_reg,        "VORN (register)",          "111100100D11nnnndddd0001NQM1mmmm") // ASIMD
INST(asimd_VEOR_reg,        "VEOR (register)",          "111100110D00nnnndddd0001NQM1mmmm") // ASIMD
INST(asimd_VBSL,            "VBSL",                     "111100110D01nnnndddd0001NQM1mmmm") // ASIMD
INST(asimd_VBIT,            "VBIT",                     "111100110D10nnnndddd0001NQM1mmmm") // ASIMD
INST(asimd_VBIF,            "VBIF",                     "111100110D11nnnndddd0001NQM1mmmm") // ASIMD
INST(asimd_VHSUB,           "VHSUB",                    "1111001U0Dzznnnndddd0010NQM0mmmm") // ASIMD
INST(asimd_VQSUB,           "VQSUB",                    "1111001U0Dzznnnndddd0010NQM1mmmm") // ASIMD
//INST(asimd_VCGT_reg,        "VCGT (register)",          "1111001U0-CC--------0011---0----") // ASIMD
//INST(asimd_VCGE_reg,        "VCGE (register)",          "1111001U0-CC--------0011---1----") // ASIMD
//INST(asimd_VSHL_reg,        "VSHL (register)",          "1111001U0-CC--------0100---0----") // ASIMD
//INST(asimd_VQSHL_reg,       "VQSHL (register)",         "1111001U0-CC--------0100---1----") // ASIMD
//INST(asimd_VRSHL,           "VRSHL",                    "1111001U0-CC--------0101---0----") // ASIMD
//INST(asimd_VQRSHL,          "VQRSHL",                   "1111001U0-CC--------0101---1----") // ASIMD
//INST(asimd_VMAX,            "VMAX/VMIN",                "1111001U0-CC--------0110---B----") // ASIMD
//INST(asimd_VABD,            "VABD/VABDL",               "1111001U0-CC--------0111---0----") // ASIMD
//INST(asimd_VABA,            "VABA/ABAL",                "1111001U0-CC--------0111---1----") // ASIMD
//INST(asimd_VADD_int,        "VADD (integer)",           "111100100-CC--------1000---0----") // ASIMD
//INST(asimd_VSUB_int,        "VAND (integer)",           "111100110-CC--------1000---0----") // ASIMD
//INST(asimd_VTST,            "VTST",                     "111100100-CC--------1000---1----") // ASIMD
//INST(asimd_VCEQ_reg,        "VCEG (register)",          "111100110-CC--------1000---1----") // ASIMD
//INST(asimd_VMLA,            "VMLA/VMLAL/VMLS/VMLSL",    "1111001U0-CC--------1001---0----") // ASIMD
//INST(asimd_VMUL,            "VMUL/VMULL",               "1111001U0-CC--------1001---1----") // ASIMD
//INST(asimd_VPMAX,           "VPMAX/VPMIN",              "1111001U0-CC--------1010---B----") // ASIMD
//INST(asimd_VQDMULH,         "VQDMULH",                  "111100100-CC--------1011---0----") // ASIMD
//INST(asimd_VQRDMULH,        "VQRDMULH",                 "111100110-CC--------1011---0----") // ASIMD
//INST(asimd_VPADD,           "VPADD",                    "1111001U0-CC--------1011---1----") // ASIMD
//INST(asimd_VFMA,            "VFMA/VFMS",                "111100100-CC--------1100---1----") // ASIMD
//INST(asimd_VADD_float,      "VADD (floating-point)",    "111100100-0C--------1101---0----") // ASIMD
//INST(asimd_VSUB_float,      "VSUB (floating-point)",    "111100100-1C--------1101---0----") // ASIMD
//INST(asimd_VPADD_float,     "VPADD (floating-point)",   "111100110-0C--------1101---0----") // ASIMD
//INST(asimd_VABD_float,      "VABD (floating-point)",    "111100110-1C--------1101---0----") // ASIMD
//INST(asimd_VMLA_float,      "VMLA (floating-point)",    "111100100-CC--------1101---1----") // ASIMD
//INST(asimd_VMUL_float,      "VMUL (floating-point)",    "111100110-0C--------1101---1----") // ASIMD
//INST(asimd_VCEQ_reg,        "VCEQ (register)",          "111100100-0C--------1110---0----") // ASIMD
//INST(asimd_VCGE_reg,        "VCGE (register)",          "111100110-0C--------1110---0----") // ASIMD
//INST(asimd_VCGT_reg,        "VCGT (register)",          "111100110-1C--------1110---0----") // ASIMD
//INST(asimd_VACGE,           "VACGE",                    "111100110-CC--------1110---1----") // ASIMD
//INST(asimd_VMAX_float,      "VMAX (floating-point)",    "111100100-CC--------1111---0----") // ASIMD
//INST(asimd_VPMAX_float,     "VMIN (floating-point)",    "111100110-CC--------1111---0----") // ASIMD
//INST(asimd_VRECPS,          "VRECPS",                   "111100100-0C--------1111---1----") // ASIMD
//INST(asimd_VRSQRTS,         "VRSQRTS",                  "111100100-1C--------1111---1----") // ASIMD

// Two registers and a scalar
//INST(asimd_VMLA_scalar,     "VMLA (scalar)",            "1111001U1-BB--------0x0x-1-0----") // ASIMD
//INST(asimd_VMLAL_scalar,    "VMLAL (scalar)",           "1111001U1-BB--------0x10-1-0----") // ASIMD
//INST(asimd_VQDMLAL,         "VQDMLAL/VQDMLSL",          "111100101-BB--------0x11-1-0----") // ASIMD
//INST(asimd_VMUL_scalar,     "VMUL (scalar)",            "1111001U1-BB--------100x-1-0----") // ASIMD
//INST(asimd_VMULL_scalar,    "VMULL (scalar)",           "1111001U1-BB--------1010-1-0----") // ASIMD
//INST(asimd_VQDMULL,         "VQDMULL",                  "111100101-BB--------1011-1-0----") // ASIMD
//INST(asimd_VQDMULH,         "VQDMULH",                  "1111001U1-BB--------1100-1-0----") // ASIMD
//INST(asimd_VQRDMULH,        "VQRDMULH",                 "1111001U1-BB--------1101-1-0----") // ASIMD

// Two registers and a shift amount
//INST(asimd_SHR,             "SHR",                      "1111001U1-vvv-------0000LB-1----") // ASIMD
//INST(asimd_SRA,             "SRA",                      "1111001U1-vvv-------0001LB-1----") // ASIMD
//INST(asimd_VRSHR,           "VRSHR",                    "1111001U1-vvv-------0010LB-1----") // ASIMD
//INST(asimd_VRSRA,           "VRSRA",                    "1111001U1-vvv-------0011LB-1----") // ASIMD
//INST(asimd_VSRI,            "VSRI",                     "111100111-vvv-------0100LB-1----") // ASIMD
//INST(asimd_VSHL,            "VSHL",                     "111100101-vvv-------0101LB-1----") // ASIMD
//INST(asimd_VSLI,            "VSLI",                     "111100111-vvv-------0101LB-1----") // ASIMD
//INST(asimd_VQSHL,           "VQSHL" ,                   "1111001U1-vvv-------011xLB-1----") // ASIMD
//INST(asimd_VSHRN,           "VSHRN",                    "111100101-vvv-------100000-1----") // ASIMD
//INST(asimd_VRSHRN,          "VRSHRN",                   "111100101-vvv-------100001-1----") // ASIMD
//INST(asimd_VQSHRUN,         "VQSHRUN",                  "111100111-vvv-------100000-1----") // ASIMD
//INST(asimd_VQRSHRUN,        "VQRSHRUN",                 "111100111-vvv-------100001-1----") // ASIMD
//INST(asimd_VQSHRN,          "VQSHRN",                   "1111001U1-vvv-------100100-1----") // ASIMD
//INST(asimd_VQRSHRN,         "VQRSHRN",                  "1111001U1-vvv-------100101-1----") // ASIMD
//INST(asimd_SHLL,            "SHLL",                     "1111001U1-vvv-------101000-1----") // ASIMD
//INST(asimd_VCVT_fixed,      "VCVT (fixed-point)",       "1111001U1-vvv-------111x0B-1----") // ASIMD

// Two registers, miscellaneous
//INST(asimd_VREV64,          "VREV64",                   "111100111-11--00----00000x-0----") // ASIMD
//INST(asimd_VREV32,          "VREV32",                   "111100111-11--00----00001x-0----") // ASIMD
//INST(asimd_VREV16,          "VREV16",                   "111100111-11--00----00010x-0----") // ASIMD
//INST(asimd_VPADDL,          "VPADDL",                   "111100111-11--00----0010xx-0----") // ASIMD
INST(asimd_VCLS,            "VCLS",                     "111100111D11zz00dddd01000QM0mmmm") // ASIMD
INST(asimd_VCLZ,            "VCLZ",                     "111100111D11zz00dddd01001QM0mmmm") // ASIMD
INST(asimd_VCNT,            "VCNT",                     "111100111D11zz00dddd01010QM0mmmm") // ASIMD
//INST(asimd_VMVN_reg,        "VMVN_reg",                 "111100111-11--00----01011x-0----") // ASIMD
//INST(asimd_VPADAL,          "VPADAL",                   "111100111-11--00----0110xx-0----") // ASIMD
//INST(asimd_VQABS,           "VQABS",                    "111100111-11--00----01110x-0----") // ASIMD
//INST(asimd_VQNEG,           "VQNEG",                    "111100111-11--00----01111x-0----") // ASIMD
//INST(asimd_VCGT_zero,       "VCGT (zero)",              "111100111-11--01----0x000x-0----") // ASIMD
//INST(asimd_VCGE_zero,       "VCGE (zero)",              "111100111-11--01----0x001x-0----") // ASIMD
//INST(asimd_VCEQ_zero,       "VCEQ (zero)",              "111100111-11--01----0x010x-0----") // ASIMD
//INST(asimd_VCLE_zero,       "VCLE (zero)",              "111100111-11--01----0x011x-0----") // ASIMD
//INST(asimd_VCLT_zero,       "VCLT (zero)",              "111100111-11--01----0x100x-0----") // ASIMD
//INST(asimd_VABS,            "VABS",                     "111100111-11--01----0x110x-0----") // ASIMD
INST(asimd_VNEG,            "VNEG",                     "111100111D11zz01dddd0F111QM0mmmm") // ASIMD
INST(asimd_VSWP,            "VSWP",                     "111100111D110010dddd00000QM0mmmm") // ASIMD
//INST(asimd_VTRN,            "VTRN",                     "111100111-11--10----00001x-0----") // ASIMD
//INST(asimd_VUZP,            "VUZP",                     "111100111-11--10----00010x-0----") // ASIMD
//INST(asimd_VZIP,            "VZIP",                     "111100111-11--10----00011x-0----") // ASIMD
//INST(asimd_VMOVN,           "VMOVN",                    "111100111-11--10----001000-0----") // ASIMD
//INST(asimd_VQMOVUN,         "VQMOVUN",                  "111100111-11--10----001001-0----") // ASIMD
//INST(asimd_VQMOVN,          "VQMOVN",                   "111100111-11--10----00101x-0----") // ASIMD
//INST(asimd_VSHLL_max,       "VSHLL_max",                "111100111-11--10----001100-0----") // ASIMD
//INST(asimd_VCVT_half,       "VCVT (half-precision)",    "111100111-11--10----011x00-0----") // ASIMD
//INST(asimd_VRECPE,          "VRECPE",                   "111100111-11--11----010x0x-0----") // ASIMD
//INST(asimd_VRSQRTE,         "VRSQRTE",                  "111100111-11--11----010x1x-0----") // ASIMD
//INST(asimd_VCVT_integer,    "VCVT (integer)",           "111100111-11--11----011xxx-0----") // ASIMD

// One register and modified immediate
INST(asimd_VMOV_imm,        "VBIC, VMOV, VMVN, VORR (immediate)",  "1111001a1D000bcdVVVVmmmm0Qo1efgh") // ASIMD

// Advanced SIMD load/store structures
INST(v8_VST_multiple,       "VST{1-4} (multiple)",      "111101000D00nnnnddddxxxxzzaammmm") // v8
INST(v8_VLD_multiple,       "VLD{1-4} (multiple)",      "111101000D10nnnnddddxxxxzzaammmm") // v8
INST(arm_UDF,               "UNALLOCATED",              "111101000--0--------1011--------") // v8
INST(arm_UDF,               "UNALLOCATED",              "111101000--0--------11----------") // v8
//INST(arm_UDF,               "UNALLOCATED",              "111101001-00--------11----------") // v8
//INST(v8_VLD_all_lanes,      "VLD{1-4} (all lanes)",     "111101001D10nnnndddd11nnzzTammmm") // v8
//INST(arm_UDF,               "UNALLOCATED",              "111101001-10--------1110---1----") // v8
//INST(v8_VST_single,         "VST{1-4} (single)",        "111101001D00nnnnddddzzNNaaaammmm") // v8
//INST(v8_VLD_single,         "VLD{1-4} (single)",        "111101001D10nnnnddddzzNNaaaammmm") // v8
