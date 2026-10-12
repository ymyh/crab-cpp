option("enable-string")
    set_default(false)
    set_showmenu(true)
    add_defines("CRAB_CPP_ENABLE_STRING")
option_end()

option("enable-backtrace")
    set_default(false)
    set_showmenu(true)
    add_defines("CRAB_CPP_ENABLE_BACKTRACE")
option_end()

option("enable-test")
    set_default(false)
    set_showmenu(true)
option_end()

add_rules("mode.debug", "mode.release")
-- Make dependency packages (gtest, utf8proc) inherit the configured toolchain
-- (e.g. mingw/gcc or clang-cl) instead of falling back to the MSVC host toolchain.
set_policy("package.inherit_external_configs", true)
add_requires("gtest", {configs = {main = true}})
add_requires("utf8proc")

local tc = get_config("toolchain")

target("crab_cpp")
    set_default(true)
    set_kind("moduleonly")
    set_languages("c++23", "c11")
    set_policy("build.c++.modules", true)

    add_options("enable-string", "enable-backtrace")

    if has_config("enable-string") then
        add_packages("utf8proc")
        add_files("src/string.cppm", {public = true})
    end

    add_files("src/match.cppm", "src/mod.cppm", "src/mutex.cppm", "src/option.cppm", "src/panic.cppm", "src/result.cppm", {public = true})

    if is_os("windows") and (tc == nil or tc == "clang-cl" or tc == "msvc") then
        add_cxxflags("/utf-8")
    end

target("crab_cpp_test")
    set_enabled(has_config("enable-test"))
    set_kind("binary")
    set_languages("c++23", "c11")
    set_policy("build.c++.modules", true)

    add_options("enable-string", "enable-backtrace")

    if has_config("enable-string") then
        add_packages("utf8proc")
        add_files("src/string.cppm")
    end

    add_packages("gtest")
    add_files("src/match.cppm", "src/mod.cppm", "src/mutex.cppm", "src/option.cppm", "src/panic.cppm", "src/result.cppm")
    add_files("tests/*.cpp")
    add_includedirs("include")

    add_links("gtest_main")

    if is_os("windows") and (tc == nil or tc == "clang-cl" or tc == "msvc") then
        add_ldflags("/subsystem:console")
        add_cxxflags("/utf-8")
    end

    -- MinGW/GCC workarounds for libstdc++ `import std` (same as CMakeLists.txt):
    --  - std::__open_terminal/std::__write_to_terminal live in libstdc++exp;
    --  - GCC PR116746 emits duplicate symbols for static locals from libstdc++
    --    headers when using `import std` together with std::print/std::format.
    on_config(function (target)
        if target:is_plat("windows", "mingw", "msys") and target:has_tool("cc", "gcc") then
            target:add("links", "stdc++exp")
            target:add("ldflags", "-Wl,--allow-multiple-definition")
        end
    end)