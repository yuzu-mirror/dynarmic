cmake_minimum_required(VERSION 3.4.1)
project(dynarmic CXX)

# Dynarmic project options
option(DYNARMIC_USE_SYSTEM_BOOST "Use the system boost libraries" ON)
option(DYNARMIC_USE_LLVM "Support disassembly of jitted x86_64 code using LLVM" OFF)

# Set hard requirements for C++
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warn on CMake API deprecations
set(CMAKE_WARN_DEPRECATED ON)

# Disable in-source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(SEND_ERROR "In-source builds are not allowed.")
endif()

# Signify where the module directories are
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

# Compiler flags
if (MSVC)
    add_compile_options(/W3 /MP /Zi /Zo /EHsc /WX)
    add_compile_options(/DNOMINMAX)
else()
    add_compile_options(-Wall
                        -Werror
                        -Wextra
                        -Wcast-qual
                        -pedantic
                        -pedantic-errors
                        -Wfatal-errors
                        -Wno-unused-parameter
                        -Wno-missing-braces)
    add_compile_options(-fno-operator-names)

    if (ARCHITECTURE_x86_64)
        add_compile_options(-msse4.1)
    endif()
endif()

# Arch detection
include(DetectArchitecture)
if (MSVC)
    detect_architecture("_M_AMD64" x86_64)
    detect_architecture("_M_IX86" x86)
    detect_architecture("_M_ARM" ARM)
else()
    detect_architecture("__x86_64__" x86_64)
    detect_architecture("__i386__" x86)
    detect_architecture("__arm__" ARM)
endif()
if (NOT DEFINED ARCHITECTURE)
    set(ARCHITECTURE "GENERIC")
    set(ARCHITECTURE_GENERIC 1)
    add_definitions(-DARCHITECTURE_GENERIC=1)
endif()
message(STATUS "Target architecture: ${ARCHITECTURE}")

# Include Boost
if(DYNARMIC_USE_SYSTEM_BOOST)
    find_package(Boost 1.57.0 REQUIRED)
else()
    if(NOT Boost_INCLUDE_DIRS)
        message(FATAL_ERROR "Please provide a path to a boost installation using Boost_INCLUDE_DIRS")
    endif()
endif()
include_directories(${Boost_INCLUDE_DIRS})

# Include Catch
include_directories(externals/catch)
enable_testing(true)  # Enables unit-testing.

# Include Xbyak
include_directories(externals/xbyak/xbyak)

# Include LLVM
if (DYNARMIC_USE_LLVM)
    find_package(LLVM REQUIRED CONFIG)
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(-DDYNARMIC_USE_LLVM ${LLVM_DEFINITIONS})
    llvm_map_components_to_libnames(llvm_libs x86desc x86disassembler)
endif()

# Pull in externals CMakeLists for libs where available
add_subdirectory(externals)

# Dynarmic project files
add_subdirectory(src)
add_subdirectory(tests)
