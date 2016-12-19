set(SRCS
    arm/fuzz_arm.cpp
    arm/fuzz_thumb.cpp
    arm/test_arm_disassembler.cpp
    arm/test_thumb_instructions.cpp
    main.cpp
    rand_int.h
    skyeye_interpreter/dyncom/arm_dyncom_dec.cpp
    skyeye_interpreter/dyncom/arm_dyncom_interpreter.cpp
    skyeye_interpreter/dyncom/arm_dyncom_thumb.cpp
    skyeye_interpreter/dyncom/arm_dyncom_trans.cpp
    skyeye_interpreter/skyeye_common/armstate.cpp
    skyeye_interpreter/skyeye_common/armsupp.cpp
    skyeye_interpreter/skyeye_common/vfp/vfp.cpp
    skyeye_interpreter/skyeye_common/vfp/vfpdouble.cpp
    skyeye_interpreter/skyeye_common/vfp/vfpinstr.cpp
    skyeye_interpreter/skyeye_common/vfp/vfpsingle.cpp
    )

set(HEADERS
    skyeye_interpreter/dyncom/arm_dyncom_dec.h
    skyeye_interpreter/dyncom/arm_dyncom_interpreter.h
    skyeye_interpreter/dyncom/arm_dyncom_run.h
    skyeye_interpreter/dyncom/arm_dyncom_thumb.h
    skyeye_interpreter/dyncom/arm_dyncom_trans.h
    skyeye_interpreter/skyeye_common/arm_regformat.h
    skyeye_interpreter/skyeye_common/armstate.h
    skyeye_interpreter/skyeye_common/armsupp.h
    skyeye_interpreter/skyeye_common/vfp/asm_vfp.h
    skyeye_interpreter/skyeye_common/vfp/vfp.h
    skyeye_interpreter/skyeye_common/vfp/vfp_helper.h
    )

include(CreateDirectoryGroups)
create_directory_groups(${SRCS} ${HEADERS})

add_executable(dynarmic_tests ${SRCS})
target_link_libraries(dynarmic_tests dynarmic ${llvm_libs})
set_target_properties(dynarmic_tests PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(dynarmic_tests PRIVATE . ../src)
target_compile_options(dynarmic PRIVATE ${DYNARMIC_CXX_FLAGS})

add_test(dynarmic_tests dynarmic_tests)
