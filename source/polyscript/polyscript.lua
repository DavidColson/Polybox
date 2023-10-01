
group "polyscript"

project "polyscript"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	exceptionhandling "Off"
	rtti "Off"
	files 
	{
		"*.cpp",
		"*.h",
		"*.inl",
		"polyscript.lua"
	}
	includedirs
	{
		"../../source/common_lib/source/"
	}
	links
	{
		"common_lib"
	}
	filter { "system:windows", "configurations:Debug*" }
		links { "dbghelp" }
	if ASAN_Enabled then
        buildoptions { "/fsanitize=address" }
        flags { "NoIncrementalLink" }
        editandcontinue "Off"
	end

project "polyscript_tests"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	exceptionhandling "Off"
	rtti "Off"
	debugdir "test"
	files 
	{
		"test/main.cpp",
		"test/compiler_explorer.h",
		"test/compiler_explorer.cpp",
		"test/tests_framework.h",
		"test/tests_framework.cpp",
		"test/test.ps",
	}
	includedirs
	{
		"../common_lib/source/",
		"../third_party/imgui",
		"../../lib/SDL2-2.0.8/include",
		"../third_party/bgfx/include",
		"../third_party/bx/include",
		""
	}
	links
	{
		"bgfx",
		"bimg",
		"bimg_decode",
		"bx",
		"SDL2",
		"SDL2main",
		"imgui",
		"common_lib",
		"polyscript"
	}
	filter "platforms:x86_64"
		libdirs { "../../lib/SDL2-2.0.8/lib/x64" }
	filter "platforms:x86"
		libdirs { "../../lib/SDL2-2.0.8/lib/x86" }
	filter { "system:windows", "configurations:Debug*" }
		links { "dbghelp" }
	if ASAN_Enabled then
		buildoptions { "/fsanitize=address" }
		flags { "NoIncrementalLink" }
		editandcontinue "Off"
	end
	filter "action:vs*"
		includedirs { "../third_party/bx/include/compat/msvc" }
	filter { "system:windows", "action:gmake" }
		includedirs { "../third_party/bx/include/compat/mingw" }
	filter { "system:macosx" }
		includedirs { "../third_party/bx/include/compat/osx" }
		buildoptions { "-x objective-c++" }

group ""