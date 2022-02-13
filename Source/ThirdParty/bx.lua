// Copyright 2020-2022 David Colson. All rights reserved.

group "ThirdParty/bx"

project "bx"
    kind "StaticLib"
    language "C++"
    cppdialect "C++14"
    exceptionhandling "Off"
    rtti "Off"
    defines "__STDC_FORMAT_MACROS"
    files
    {
        "bx/include/bx/*.h",
        "bx/include/bx/inline/*.inl",
        "bx/src/*.cpp"
    }
    excludes
    {
        "bx/src/amalgamated.cpp",
        "bx/src/crtnone.cpp"
    }
    includedirs
    {
        "bx/3rdparty",
        "bx/include"
    }
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