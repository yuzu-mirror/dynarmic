set(SRCS
    logging/log.cpp
    memory_util.cpp
    string_util.cpp
    x64/abi.cpp
    x64/cpu_detect.cpp
    x64/emitter.cpp
    )

set(HEADERS
    assert.h
    bit_set.h
    bit_util.h
    code_block.h
    common_types.h
    logging/log.h
    memory_util.h
    mp.h
    scope_exit.h
    string_util.h
    x64/abi.h
    x64/cpu_detect.h
    x64/emitter.h
    )

source_group(common FILES ${SRCS} ${HEADERS})
add_library(dynarmic_common STATIC ${SRCS} ${HEADERS})
set_target_properties(dynarmic_common PROPERTIES LINKER_LANGUAGE CXX)
