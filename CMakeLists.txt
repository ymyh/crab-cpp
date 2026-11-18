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

project(crab_cpp VERSION 0.4.1 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Set C++23 and enable modules when built as the top-level project;
# consumers still receive cxx_std_23 via target_compile_features().
if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Let CMake add the standard library `std` module dependency for GCC and Clang
# using libstdc++/libc++. clang-cl/MSVC handle `import std` themselves, and the
# Visual Studio generator does not support CMake-built std modules.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"
            AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    set(CMAKE_CXX_MODULE_STD ON)
endif()

# Options
option(CRAB_CPP_ENABLE_STRING "Enable String sub-module" OFF)
option(CRAB_CPP_ENABLE_BACKTRACE "Enable std::backtrace support" OFF)
option(CRAB_CPP_ENABLE_TEST "Enable building tests" OFF)

# Fetch utf8proc only when the optional String sub-module is enabled
if (CRAB_CPP_ENABLE_STRING)
    if (NOT TARGET utf8proc AND NOT TARGET utf8proc::utf8proc)
        find_package(utf8proc CONFIG QUIET)
    endif()

    if (NOT TARGET utf8proc AND NOT TARGET utf8proc::utf8proc)
        include(FetchContent)
        FetchContent_Declare(
            utf8proc
            URL https://github.com/JuliaStrings/utf8proc/releases/download/v2.12.0/utf8proc-2.12.0.tar.gz
            URL_HASH SHA256=a393fbef160835fb315bc3e91ba8d86f7a73a7cec9e6198b6c60b848b498bfeb
        )
        FetchContent_MakeAvailable(utf8proc)
    endif()
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
add_library(crab_cpp::crab_cpp ALIAS crab_cpp)

target_sources(crab_cpp
    PUBLIC
        FILE_SET CXX_MODULES FILES ${CRAB_CPP_MODULES}
)

target_include_directories(crab_cpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(crab_cpp PUBLIC cxx_std_23)
set_target_properties(crab_cpp PROPERTIES CXX_EXTENSIONS OFF)

if (MSVC)
    target_compile_options(crab_cpp PUBLIC /utf-8)
endif()

# Propagate feature macros to consumers via PUBLIC compile definitions
if (CRAB_CPP_ENABLE_STRING)
    target_compile_definitions(crab_cpp PUBLIC CRAB_CPP_ENABLE_STRING)
    if (TARGET utf8proc::utf8proc)
        target_link_libraries(crab_cpp PRIVATE utf8proc::utf8proc)
    else()
        target_link_libraries(crab_cpp PRIVATE utf8proc)
    endif()
endif()

if (CRAB_CPP_ENABLE_BACKTRACE)
    target_compile_definitions(crab_cpp PUBLIC CRAB_CPP_ENABLE_BACKTRACE)
endif()

# MinGW/libstdc++ workarounds for `import std` + std::print/std::stacktrace:
#  - std::__open_terminal/std::__write_to_terminal live in libstdc++exp;
#  - std::mutex on MinGW posix threads requires pthread.
if (MINGW)
    target_link_libraries(crab_cpp PUBLIC stdc++exp pthread)
endif()

# Install and package config (only when top-level so FetchContent consumers are unaffected)
if (PROJECT_IS_TOP_LEVEL)
    install(
        TARGETS crab_cpp
        EXPORT crab_cppTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILE_SET CXX_MODULES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/crab_cpp/modules"
    )

    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    install(
        EXPORT crab_cppTargets
        FILE crab_cppTargets.cmake
        NAMESPACE crab_cpp::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/crab_cpp"
    )

    set(_crab_cpp_find_utf8proc "")
    if (CRAB_CPP_ENABLE_STRING)
        set(_crab_cpp_find_utf8proc "find_dependency(utf8proc CONFIG)\n")
    endif()

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/crab_cppConfig.cmake.in"
        "@PACKAGE_INIT@\n\n"
        "include(CMakeFindDependencyMacro)\n"
        "${_crab_cpp_find_utf8proc}"
        "include(\"\${CMAKE_CURRENT_LIST_DIR}/crab_cppTargets.cmake\")\n"
        "check_required_components(crab_cpp)\n"
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_BINARY_DIR}/crab_cppConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/crab_cppConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/crab_cpp"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/crab_cppConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/crab_cppConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/crab_cppConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/crab_cpp"
    )
endif()

# Tests
if (CRAB_CPP_ENABLE_TEST)
    enable_testing()

    if (NOT TARGET GTest::gtest_main)
        find_package(GTest CONFIG QUIET)
    endif()

    if (NOT TARGET GTest::gtest_main)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/releases/download/v1.18.0/googletest-1.18.0.tar.gz
            URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)

    add_executable(crab_cpp_test ${TEST_SOURCES})
    target_compile_definitions(crab_cpp_test PRIVATE CRAB_CPP_ENABLE_TEST)
    target_link_libraries(crab_cpp_test PRIVATE crab_cpp::crab_cpp GTest::gtest_main)

    # GCC PR116746 emits static locals from libstdc++ headers with external
    # linkage, producing duplicate symbol errors for any program that uses
    # both `import std` and std::print/std::format.
    if (MINGW)
        target_link_options(crab_cpp_test PRIVATE "-Wl,--allow-multiple-definition")
    endif()

    add_test(NAME crab_cpp_test COMMAND crab_cpp_test)
endif()
