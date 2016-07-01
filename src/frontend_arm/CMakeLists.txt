set(SRCS
    arm_disassembler.cpp
    )

set(HEADERS
    arm_disassembler.h
    arm_types.h
    decoder/arm.h
    decoder/decoder_detail.h
    decoder/thumb1.h
    frontend_arm.h
    ir/ir.cpp
    ir/ir.h
    ir_emitter.cpp
    ir_emitter.h
    translate_thumb.h
    ir/opcodes.h)

source_group(frontend_arm FILES ${SRCS} ${HEADERS})
add_library(dynarmic_frontend_arm STATIC ${SRCS} ${HEADERS})
target_link_libraries(dynarmic_frontend_arm dynarmic_common)
set_target_properties(dynarmic_frontend_arm PROPERTIES LINKER_LANGUAGE CXX)
