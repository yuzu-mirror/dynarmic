cmake_minimum_required(VERSION 3.4.1)
project(dynarmic)

# Dynarmic project options
option(DYNARMIC_USE_SYSTEM_BOOST "Use the system boost libraries" ON)

# Compiler flags
if (NOT MSVC)
    add_compile_options(--std=c++14 -Wall -Werror -Wextra -pedantic -Wfatal-errors -Wno-unused-parameter -Wno-missing-braces)
    if (ARCHITECTURE_x86_64)
        add_compile_options(-msse4.1)
    endif()
else()
    add_compile_options(/W3 /MP /Zi /Zo /EHsc /WX)
endif()

# This function should be passed a list of all files in a target. It will automatically generate
# file groups following the directory hierarchy, so that the layout of the files in IDEs matches the
# one in the filesystem.
function(create_directory_groups)
    # Place any files that aren't in the source list in a separate group so that they don't get in
    # the way.
    source_group("Other Files" REGULAR_EXPRESSION ".")

    foreach(file_name ${ARGV})
        get_filename_component(dir_name "${file_name}" PATH)
        # Group names use '\' as a separator even though the entire rest of CMake uses '/'...
        string(REPLACE "/" "\\" group_name "${dir_name}")
        source_group("${group_name}" FILES "${file_name}")
    endforeach()
endfunction()

# Arch detection
include(CheckSymbolExists)
function(detect_architecture symbol arch)
    if (NOT DEFINED ARCHITECTURE)
        set(CMAKE_REQUIRED_QUIET 1)
        check_symbol_exists("${symbol}" "" ARCHITECTURE_${arch})
        unset(CMAKE_REQUIRED_QUIET)

        # The output variable needs to be unique across invocations otherwise
        # CMake's crazy scope rules will keep it defined
        if (ARCHITECTURE_${arch})
            set(ARCHITECTURE "${arch}" PARENT_SCOPE)
            set(ARCHITECTURE_${arch} 1 PARENT_SCOPE)
            add_definitions(-DARCHITECTURE_${arch}=1)
        endif()
    endif()
endfunction()
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

# Dynarmic project files
add_subdirectory(src)
add_subdirectory(tests)
