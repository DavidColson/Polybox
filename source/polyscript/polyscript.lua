
group "polyscript"

project "polyscript"
	kind "StaticLib"
	language "C++"
	cppdialect "C++14"
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
	cppdialect "C++14"
	exceptionhandling "Off"
	rtti "Off"
	debugdir "test"
	files 
	{
		"test/main.cpp",
		"test/test.ps",
	}
	includedirs
	{
		"../../source/common_lib/source/",
		""
	}
	links
	{
		"common_lib",
		"polyscript"
	}
	filter { "system:windows", "configurations:Debug*" }
		links { "dbghelp" }
	if ASAN_Enabled then
		buildoptions { "/fsanitize=address" }
		flags { "NoIncrementalLink" }
		editandcontinue "Off"
	end

group ""