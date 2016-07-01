set(SRCS
    arm_disassembler.cpp
    ir/ir.cpp
    ir_emitter.cpp
    )

set(HEADERS
    arm_disassembler.h
    arm_types.h
    decoder/arm.h
    decoder/decoder_detail.h
    decoder/thumb1.h
    frontend_arm.h
    ir/ir.h
    ir/opcodes.h
    ir_emitter.h
    translate_thumb.h
    )

source_group(frontend_arm FILES ${SRCS} ${HEADERS})
add_library(dynarmic_frontend_arm STATIC ${SRCS} ${HEADERS})
target_link_libraries(dynarmic_frontend_arm dynarmic_common)
set_target_properties(dynarmic_frontend_arm PROPERTIES LINKER_LANGUAGE CXX)
