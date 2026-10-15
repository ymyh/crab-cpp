cmake_minimum_required(VERSION 3.31.6)

# Enable experimental `import std;` support. This must be set before project()
# so that CMake's compiler probe can see it. GCC/libstdc++ does not ship a
# prebuilt `std` module, so CMake builds it from bits/std.cc (only supported by
# the Ninja generators). The gate value is tied to the CMake version; update it
# when upgrading CMake (a wrong value produces a warning that names the gate).
if (CMAKE_VERSION VERSION_GREATER_EQUAL "4.4")
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
else()
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
endif()

project(crab_cpp LANGUAGES CXX)

# Set C++23 and enable modules
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Let CMake add the standard library `std` module dependency for GCC and Clang
# using libstdc++/libc++. clang-cl/MSVC handle `import std` themselves, and the
# Visual Studio generator does not support CMake-built std modules.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"
            AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    set(CMAKE_CXX_MODULE_STD ON)
endif()

# Enable modules (requires CMake >= 3.28 for proper support, but we use basic flags)
if (MSVC)
    add_compile_options(/experimental:module /utf-8)
endif()

# Options
option(CRAB_CPP_ENABLE_STRING "Enable String sub-module" OFF)
option(CRAB_CPP_ENABLE_BACKTRACE "Enable std::backtrace support" OFF)
option(CRAB_CPP_ENABLE_TEST "Enable building tests" OFF)

# Define macros if options are enabled
if (CRAB_CPP_ENABLE_STRING)
    add_definitions(-DCRAB_CPP_ENABLE_STRING)

    include(FetchContent)
    FetchContent_Declare(
        utf8proc
        URL https://github.com/JuliaStrings/utf8proc/releases/download/v2.12.0/utf8proc-2.12.0.tar.gz
        URL_HASH SHA256=a393fbef160835fb315bc3e91ba8d86f7a73a7cec9e6198b6c60b848b498bfeb
    )
    FetchContent_MakeAvailable(utf8proc)
endif()

if (CRAB_CPP_ENABLE_BACKTRACE)
    add_definitions(-DCRAB_CPP_ENABLE_BACKTRACE)
endif()

# Gather module sources (*.cppm in src). string.cppm is optional and only built
# when the String sub-module is enabled (it requires utf8proc).
set(CRAB_CPP_MODULES
    src/match.cppm
    src/mod.cppm
    src/mutex.cppm
    src/option.cppm
    src/panic.cppm
    src/result.cppm
)
if (CRAB_CPP_ENABLE_STRING)
    list(APPEND CRAB_CPP_MODULES src/string.cppm)
endif()

# Add main library target (using modules). Module sources must be declared as a
# CXX_MODULES file set, otherwise the Ninja generator cannot build them.
add_library(crab_cpp)
target_sources(crab_cpp
    PUBLIC
        FILE_SET CXX_MODULES FILES ${CRAB_CPP_MODULES}
)

# Link utf8proc if enabled
if (CRAB_CPP_ENABLE_STRING)
    target_link_libraries(crab_cpp PRIVATE utf8proc)
endif()

# Set include directories
target_include_directories(crab_cpp PUBLIC include)

# Tests
if (CRAB_CPP_ENABLE_TEST)
    enable_testing()
    add_definitions(-DCRAB_CPP_ENABLE_TEST)

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/releases/download/v1.18.0/googletest-1.18.0.tar.gz
        URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
    )
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

    add_executable(crab_cpp_test ${TEST_SOURCES})
    target_link_libraries(crab_cpp_test PRIVATE crab_cpp gtest gtest_main)
    target_include_directories(crab_cpp_test PRIVATE include)

    # Link utf8proc if needed
    if (CRAB_CPP_ENABLE_STRING)
        target_link_libraries(crab_cpp_test PRIVATE utf8proc)
    endif()

    # MinGW/GCC workarounds for libstdc++ `import std` issues:
    #  - std::__open_terminal/std::__write_to_terminal live in libstdc++exp;
    #  - GCC PR116746 emits static locals from libstdc++ headers with external
    #    linkage, producing duplicate symbol errors for any program that uses
    #    both `import std` and std::print/std::format.
    if (MINGW)
        target_link_libraries(crab_cpp_test PRIVATE stdc++exp pthread)
        target_link_options(crab_cpp_test PRIVATE "-Wl,--allow-multiple-definition")
    endif()

    add_test(NAME crab_cpp_test COMMAND crab_cpp_test)
endif()
