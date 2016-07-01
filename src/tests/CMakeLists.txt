set(SRCS
    arm/test_arm_disassembler.cpp
    main.cpp
    arm/test_thumb_instructions.cpp
    arm/fuzz_thumb.cpp)

set(HEADERS
    )

source_group(tests FILES ${SRCS} ${HEADERS})
add_executable(dynarmic_tests ${SRCS})
target_link_libraries(dynarmic_tests dynarmic_common dynarmic_frontend_arm dynarmic_backend_x64)
set_target_properties(dynarmic_tests PROPERTIES LINKER_LANGUAGE CXX)
