set(SRCS
    emit_x64.cpp
    reg_alloc.cpp
    routines.cpp
    )

set(HEADERS
    ../interface/interface.h
    emit_x64.h
    jitstate.h
    reg_alloc.h
    routines.h
    )

source_group(frontend_x64 FILES ${SRCS} ${HEADERS})
add_library(dynarmic_backend_x64 STATIC ${SRCS} ${HEADERS})
target_link_libraries(dynarmic_backend_x64 dynarmic_common)
set_target_properties(dynarmic_backend_x64 PROPERTIES LINKER_LANGUAGE CXX)
