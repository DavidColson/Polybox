
group "third_party/imgui"

project "imgui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    exceptionhandling "Off"
    rtti "Off"
    files
    {
        "imgui/backends/imgui_impl_sdl2.cpp",
        "imgui/backends/imgui_impl_sdl2.h",
        "imgui/backends/imgui_impl_bgfx.cpp",
        "imgui/backends/imgui_impl_bgfx.h",
        "imgui/*.cpp",
        "imgui/*.h"
    }
    includedirs
    {
        "imgui/",
        "../../Lib/SDL2-2.0.8/include",
        "bgfx/include",
        "bimg/include",
        "bx/include",
        "../../source/common_lib/source/"
    }
    if ASAN_Enabled then
    filter { "system:windows", "configurations:Debug*" }
        buildoptions { "/fsanitize=address" }
        flags { "NoIncrementalLink" }
        editandcontinue "Off"
    end
    filter "action:vs*"
        defines "_CRT_SECURE_NO_WARNINGS"
    filter "action:vs*"
		includedirs { "bx/include/compat/msvc" }
	filter { "system:windows", "action:gmake" }
		includedirs { "bx/include/compat/mingw" }
	filter { "system:macosx" }
		includedirs { "bx/include/compat/osx" }
		buildoptions { "-x objective-c++" }

group ""