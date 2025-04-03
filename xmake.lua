add_rules("mode.debug", "mode.release")

target("crab_cpp")
    set_kind("static")
    set_languages("c++23", "c11")
    set_policy("build.c++.modules", true, {public = true})

    add_files("src/*.cppm", {public = true})
    add_files("src/*.c")

    add_defines("CRAB_CPP_ENABLE_STRING", "UTF8PROC_STATIC")
    add_includedirs("include", ".")

    if is_os("windows") then
        add_defines("_CRT_USE_BUILTIN_OFFSETOF")
        set_toolchains("clang-cl")
    else
        set_toolchains("clang")
    end

target("crab_cpp_dev")
    set_kind("binary")
    set_languages("c++23", "c11")
    set_policy("build.c++.modules", true)

    add_files("src/*.cppm", {public = true})
    add_files("main.cpp")
    add_files("src/*.c")

    add_defines("CRAB_CPP_ENABLE_STRING", "UTF8PROC_STATIC")
    add_includedirs("include", ".")

    if is_os("windows") then
        add_defines("_CRT_USE_BUILTIN_OFFSETOF")
        add_toolchains("clang-cl")
    else
        add_toolchains("clang")
    end