set(SRCS
    string_util.cpp
    x64/abi.cpp
    x64/cpu_detect.cpp
    x64/emitter.cpp
    )

set(HEADERS
    bit_util.h
    common_types.h
    mp.h
    string_util.h
    x64/abi.h
    x64/cpu_detect.h
    x64/emitter.h
    assert.h logging/log.h logging/log.cpp bit_set.h memory_util.h memory_util.cpp code_block.h)

source_group(common FILES ${SRCS} ${HEADERS})
add_library(dynarmic_common STATIC ${SRCS} ${HEADERS})
set_target_properties(dynarmic_common PROPERTIES LINKER_LANGUAGE CXX)
