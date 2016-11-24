# For libraries that already come with a CMakeLists file,
# simply add the directory to that file as a subdirectory
# to have CMake automatically recognize them.

if (NOT DYNARMIC_NO_BUNDLED_FMT)
    # fmtlib formatting library
    add_subdirectory(fmt)
endif()
