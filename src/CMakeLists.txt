include_directories(.)

set(SRCS
    backend_x64/emit_x64.cpp
    backend_x64/interface_x64.cpp
    backend_x64/reg_alloc.cpp
    backend_x64/routines.cpp
    common/logging/log.cpp
    common/memory_util.cpp
    common/string_util.cpp
    common/x64/abi.cpp
    common/x64/cpu_detect.cpp
    common/x64/emitter.cpp
    frontend/disassembler_arm.cpp
    frontend/ir/ir.cpp
    frontend/ir_emitter.cpp
    )

set(HEADERS
    backend_x64/emit_x64.h
    backend_x64/jitstate.h
    backend_x64/reg_alloc.h
    backend_x64/routines.h
    common/assert.h
    common/bit_set.h
    common/bit_util.h
    common/code_block.h
    common/common_types.h
    common/logging/log.h
    common/memory_util.h
    common/mp.h
    common/scope_exit.h
    common/string_util.h
    common/x64/abi.h
    common/x64/cpu_detect.h
    common/x64/emitter.h
    frontend/arm_types.h
    frontend/decoder/arm.h
    frontend/decoder/decoder_detail.h
    frontend/decoder/thumb1.h
    frontend/disassembler_arm.h
    frontend/frontend_arm.h
    frontend/ir/ir.h
    frontend/ir/opcodes.h
    frontend/ir_emitter.h
    frontend/translate_thumb.h
    interface/interface.h
    )

source_group(dynarmic FILES ${SRCS} ${HEADERS})
add_library(dynarmic STATIC ${SRCS} ${HEADERS})
set_target_properties(dynarmic PROPERTIES LINKER_LANGUAGE CXX)
