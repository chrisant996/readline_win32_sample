local to = ".build/"..(_ACTION or "nullaction")

if _ACTION == "gmake2" then
    print("\nWARNING: gmake2 neglects to link resources.\n\n")
end

--------------------------------------------------------------------------------
local function setup_cfg(cfg)
    configuration(cfg)
        targetdir(to.."/bin/"..cfg)
        objdir(to.."/obj/")

    configuration({cfg, "x32"})
        targetsuffix("_x86")

    configuration({cfg, "x64"})
        targetsuffix("_x64")

    configuration({cfg, "arm64"})
        targetsuffix("_arm64")
end

--------------------------------------------------------------------------------
workspace("sample")
    configurations({"debug", "release"})
    platforms({"x32", "x64", "arm64"})
    location(to)

    characterset("MBCS")
    flags("NoManifest")
    staticruntime("on")
    symbols("on")
    exceptionhandling("off")
    defines("HAVE_CONFIG_H")
    defines("HANDLE_MULTIBYTE")

    setup_cfg("release")
    setup_cfg("debug")

    configuration("debug")
        rtti("on")
        optimize("off")
        defines("DEBUG")
        defines("_DEBUG")

    configuration("release")
        --rtti("off")
        rtti("on")
        optimize("full")
        defines("NDEBUG")

    configuration("vs*")
        defines("_HAS_EXCEPTIONS=0")
        defines("_CRT_SECURE_NO_WARNINGS")
        defines("_CRT_NONSTDC_NO_WARNINGS")

    configuration("gmake")
        defines("__MSVCRT_VERSION__=0x0601")
        defines("_WIN32_WINNT=0x0601")
        defines("WINVER=0x0601")
        defines("_POSIX=1")             -- so vsnprintf returns needed size
        buildoptions("-Wno-error=missing-field-initializers")
        buildoptions("-ffunction-sections")
        buildoptions("-fdata-sections")
        makesettings { "CC=gcc" }

--------------------------------------------------------------------------------
project("readline")
    language("c")
    kind("staticlib")
    defines("BUILD_READLINE")           -- so config.h can configure for readline or for the host
    includedirs("compat")
    includedirs("readline")
    includedirs("..")                   -- work around a typo in xmalloc.h that includes readline/rlstdc.h instead of just rlstdc.h
    files("readline/*.c")
    files("readline/*.h")

    excludes("readline/emacs_keymap.c") -- #included by readline/keymaps.c
    excludes("readline/vi_keymap.c")    -- #included by readline/keymaps.c

--------------------------------------------------------------------------------
project("compat")
    language("c")
    kind("staticlib")
    flags("fatalwarnings")
    includedirs("compat")
    includedirs("readline")
    includedirs("..")
    files("compat/*.c")
    files("compat/*.h")

--------------------------------------------------------------------------------
project("c_sample")
    targetname("c_sample")
    language("c")
    kind("consoleapp")
    flags("fatalwarnings")
    includedirs("compat")
    includedirs("readline")
    includedirs(".")
    links("compat")
    links("readline")
    files("c_sample.c")

--------------------------------------------------------------------------------
--[[
project("cpp_sample")
    targetname("cpp_sample")
    language("c++")
    kind("consoleapp")
    flags("fatalwarnings")
    links("compat")
    links("getopt")
    links("readline")
    links("cpp_sample.cpp")
--]]

