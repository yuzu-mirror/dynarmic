set(SRCS
    arm/fuzz_thumb.cpp
    arm/test_arm_disassembler.cpp
    arm/test_thumb_instructions.cpp
    main.cpp
    )

set(HEADERS
    )

source_group(tests FILES ${SRCS} ${HEADERS})
add_executable(dynarmic_tests ${SRCS})
target_link_libraries(dynarmic_tests dynarmic_common dynarmic_frontend_arm dynarmic_backend_x64)
set_target_properties(dynarmic_tests PROPERTIES LINKER_LANGUAGE CXX)
