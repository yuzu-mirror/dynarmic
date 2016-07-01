set(SRCS
    main.cpp
    print_tuples.h)

set(HEADERS
    )

source_group(daggen FILES ${SRCS} ${HEADERS})
add_executable(dynarmic_daggen ${SRCS})
target_link_libraries(dynarmic_daggen dynarmic_common)
set_target_properties(dynarmic_daggen PROPERTIES LINKER_LANGUAGE CXX)
