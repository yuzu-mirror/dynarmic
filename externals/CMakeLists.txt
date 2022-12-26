# Always build externals as static libraries, even when dynarmic is built as shared
if (BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# For libraries that already come with a CMakeLists file,
# simply add the directory to that file as a subdirectory
# to have CMake automatically recognize them.

# catch

if (NOT TARGET Catch2::Catch2)
    if (DYNARMIC_TESTS)
        add_library(Catch2::Catch2 INTERFACE IMPORTED GLOBAL)
        target_include_directories(Catch2::Catch2 INTERFACE catch/include)
        
        if(CMAKE_SYSTEM_NAME STREQUAL "Android")
            target_link_libraries(Catch2::Catch2 INTERFACE log)
        endif()
    endif()
endif()

# fmt

if (NOT TARGET fmt::fmt)
    # fmtlib formatting library
    option(FMT_INSTALL "" ON)
    add_subdirectory(fmt EXCLUDE_FROM_ALL)
endif()

# mcl

if (NOT TARGET merry::mcl)
    option(MCL_INSTALL "" ON)
    add_subdirectory(mcl EXCLUDE_FROM_ALL)
endif()

# oaknut

if (NOT TARGET merry::oaknut)
    if (ARCHITECTURE STREQUAL "arm64")
        add_subdirectory(oaknut EXCLUDE_FROM_ALL)
    endif()
endif()

# robin-map

if (NOT TARGET tsl::robin_map)
    add_subdirectory(robin-map EXCLUDE_FROM_ALL)
endif()

# xbyak

if (NOT TARGET xbyak::xbyak)
    if (ARCHITECTURE STREQUAL "x86" OR ARCHITECTURE STREQUAL "x86_64")
        add_subdirectory(xbyak EXCLUDE_FROM_ALL)
    endif()
endif()

# zydis

if (NOT TARGET Zydis::Zydis)
    if (ARCHITECTURE STREQUAL "x86" OR ARCHITECTURE STREQUAL "x86_64")
        option(ZYDIS_BUILD_TOOLS "" OFF)
        option(ZYDIS_BUILD_EXAMPLES "" OFF)
        option(ZYDIS_BUILD_DOXYGEN "" OFF)
        set(ZYAN_ZYCORE_PATH "${CMAKE_CURRENT_LIST_DIR}/zycore" CACHE PATH "")
        add_subdirectory(zydis EXCLUDE_FROM_ALL)
        add_library(Zydis::Zydis ALIAS Zydis)
    endif()
endif()
