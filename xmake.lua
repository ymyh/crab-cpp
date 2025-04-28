add_rules("mode.debug", "mode.release")
add_requires("gtest", {configs = {main = true}})

target("crab_cpp")
    set_kind("static")
    set_languages("c++23", "c11")
    set_policy("build.c++.modules", true, {public = true})

    add_files("src/*.cppm", {public = true})
    add_files("src/*.c", {public = true})

    add_defines("CRAB_CPP_ENABLE_STRING", "UTF8PROC_STATIC")
    add_includedirs("include", ".")

    if is_os("windows") then
        add_cxxflags("/utf-8")
        set_toolchains("clang-cl")
    elseif is_os("macosx") then
        add_toolchains("clang")
    else
        add_toolchains("gcc")
    end

target("crab_cpp_dev")
    set_kind("binary")
    set_languages("c++23", "c11")
    set_policy("build.c++.modules", true)

    add_files("src/*.cppm", {public = true})
    add_files("src/*.c")
    add_files("main.cpp")

    add_defines("CRAB_CPP_ENABLE_STRING", "UTF8PROC_STATIC")
    add_includedirs("include", ".")

    if is_os("windows") then
        add_cxxflags("/utf-8")
        add_toolchains("clang-cl")
    elseif is_os("macosx") then
        add_toolchains("clang")
    else
        add_toolchains("gcc")
    end

target("crab_cpp_test")
    set_kind("binary")
    set_languages("c++23", "c11")
    set_policy("build.c++.modules", true)
    add_packages("gtest")

    add_files("src/*.cppm", {public = true})
    add_files("src/*.c")
    add_files("tests/*.cpp")

    add_defines("CRAB_CPP_ENABLE_STRING", "UTF8PROC_STATIC")
    add_includedirs("include", ".")

    add_links("gtest_main")

    if is_os("windows") then
        add_ldflags("/subsystem:console")
        add_cxxflags("/utf-8")
        add_toolchains("clang-cl")
    elseif is_os("macosx") then
        add_toolchains("clang")
    else
        add_toolchains("gcc")
    end