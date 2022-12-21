
group "third_party/bimg"

project "bimg"
    kind "StaticLib"
    language "C++"
    cppdialect "C++14"
    exceptionhandling "Off"
    rtti "Off"
    files
    {
        "bimg/include/bimg/*.h",
        "bimg/src/image.cpp",
        "bimg/src/image_gnf.cpp",
        "bimg/src/*.h",
        "bimg/3rdparty/astc-codec/src/decoder/*"
    }
    includedirs
    {
        "bx/include",
        "bimg/include",
        "bimg/3rdparty/astc-codec",
        "bimg/3rdparty/astc-codec/include",
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

project "bimg_decode"
    kind "StaticLib"
    language "C++"
    cppdialect "C++14"
    exceptionhandling "Off"
    rtti "Off"
    includedirs 
    {
        "bx/include",
        "bimg/include",
        "bimg/3rdparty",
    }
    files 
    {
        "bimg/include/**",
        "bimg/src/image_decode.*",
    }
    if ASAN_Enabled then
    filter { "system:windows", "configurations:Debug*" }
        buildoptions { "/fsanitize=address" }
        flags { "NoIncrementalLink" }
        editandcontinue "Off"
    end
    filter { "system:linux" }
        buildoptions { "-fPIC" }
    filter "action:vs*"
		includedirs { "bx/include/compat/msvc" }
	filter { "system:windows", "action:gmake" }
		includedirs { "bx/include/compat/mingw" }
	filter { "system:macosx" }
		includedirs { "bx/include/compat/osx" }
		buildoptions { "-x objective-c++" }

group ""