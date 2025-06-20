cmake_minimum_required(VERSION 3.31.6)
project(crab_cpp LANGUAGES CXX)

# Set C++23 and enable modules
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable modules (requires CMake >= 3.28 for proper support, but we use basic flags)
if (MSVC)
    add_compile_options(/experimental:module /utf-8)
else()
    add_compile_options(-fmodules-ts)
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
        URL https://github.com/JuliaStrings/utf8proc/releases/download/v2.10.0/utf8proc-2.10.0.tar.gz
    )
    FetchContent_MakeAvailable(utf8proc)
endif()

if (CRAB_CPP_ENABLE_BACKTRACE)
    add_definitions(-DCRAB_CPP_ENABLE_BACKTRACE)
endif()

# Gather module sources (*.cppm in src)
file(GLOB_RECURSE CRAB_CPP_MODULES src/*.cppm)

# Add main library target (using modules)
add_library(crab_cpp ${CRAB_CPP_MODULES})

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
        URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
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

    add_test(NAME crab_cpp_test COMMAND crab_cpp_test)
endif()
