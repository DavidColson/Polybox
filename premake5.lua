
local BUILD_DIR = path.join("build", _ACTION)
if _OPTIONS["cc"] ~= nil then
	BUILD_DIR = BUILD_DIR .. "_" .. _OPTIONS["cc"]
end

GAME_DIR    = "game/source"
ENGINE_DIR  = "engine/source"

-- Third party locations
local SDL_DIR     = "Engine/lib/SDL2-2.0.8"
local BGFX_DIR    = "Engine/source/third_party/bgfx"
local BIMG_DIR    = "Engine/source/third_party/bimg"
local BX_DIR      = "Engine/source/third_party/bx"
local EABASE_DIR  = "Engine/source/third_party/EABase"
local EASTL_DIR   = "Engine/source/third_party/EASTL"

function setBxCompat()
	filter "action:vs*"
		includedirs { path.join(BX_DIR, "include/compat/msvc") }
	filter { "system:windows", "action:gmake" }
		includedirs { path.join(BX_DIR, "include/compat/mingw") }
	filter { "system:macosx" }
		includedirs { path.join(BX_DIR, "include/compat/osx") }
		buildoptions { "-x objective-c++" }
end

solution "polybox"
    location(BUILD_DIR)
    startproject "polybox"
    configurations { "Release", "Debug" }
    platforms "x86_64"
    architecture "x86_64"
        filter "configurations:Release"
        defines "NDEBUG"
        optimize "Full"
    filter "configurations:Debug*"
        defines { "_DEBUG", "MEMORY_TRACKING" }
        optimize "Debug"
        symbols "On"
    filter "system:macosx"
        xcodebuildsettings {
            ["MACOSX_DEPLOYMENT_TARGET"] = "10.9",
            ["ALWAYS_SEARCH_USER_PATHS"] = "YES", -- This is the minimum version of macos we'll be able to run on
        };
    
    dofile("source/Commonlib/commonlib.lua")
    dofile("source/third_party/bx.lua")
    dofile("source/third_party/bimg.lua")
    dofile("source/third_party/bgfx.lua")
    dofile("source/third_party/FreeType.lua")
    dofile("source/third_party/lua.lua")
    
    project "polybox"
        kind "WindowedApp"
        language "C++"
        cppdialect "C++20"
        exceptionhandling "Off"
        rtti "Off"
        debugdir ""
        files 
        {
            "source/*.cpp",
            "source/*.h",
            "shaders/**.fsc",
            "shaders/**.vsc",
            "shaders/**.sc",
            "shaders/**.sh"
        }
        includedirs
        {
            "source/",
            "source/Commonlib/source",
            "source/third_party/bgfx/include",
            "source/third_party/bimg/include",
            "source/third_party/bx/include",
            "source/third_party/bgfx/tools",
            "source/third_party/FreeType/include",
            "source/third_party/lua/src",
            "lib/SDL2-2.0.8/include",
        }
        links 
        { 
            "bgfx",
            "bimg",
            "bimg_decode",
            "bx",
            "FreeType",
            "lua",
            "SDL2",
            "SDL2main",
            "common_lib"
        }
        defines 
        {
            "WINDOWS_IGNORE_PACKING_MISMATCH",
            "__STDC_LIMIT_MACROS",
            "__STDC_FORMAT_MACROS",
            "__STDC_CONSTANT_MACROS",
        }
        dependson { "shaderc" }
        filter "files:**.fsc"
            buildmessage 'Compiling %{file.relpath}'
            buildcommands {
                '%{cfg.buildtarget.directory}/shaderc.exe -f "%{file.path}" -o "%{file.directory}/Bin/%{file.basename}.fbin" --type f --varyingdef "%{file.directory}/varying.def.sc" -p ps_4_0 --platform windows'
            }
            buildoutputs { '%{file.directory}/Bin/%{file.basename}.fbin' }
            buildinputs 
            { 
                '%{file.directory}/varying.def.sc',  
                '%{file.directory}/shaderlib.sh',  
                '%{file.directory}/common.sh',  
                '%{file.directory}/bgfx_shader.sh'
            }
        filter "files:**.vsc"
            buildmessage 'Compiling %{file.relpath}'
            buildcommands {
                '%{cfg.buildtarget.directory}/shaderc.exe -f "%{file.path}" -o "%{file.directory}/Bin/%{file.basename}.vbin" --type v --varyingdef "%{file.directory}/varying.def.sc" -p vs_4_0 --platform windows'
            }
            buildoutputs { '%{file.directory}/Bin/%{file.basename}.vbin' }
            buildinputs 
            { 
                '%{file.directory}/varying.def.sc',  
                '%{file.directory}/shaderlib.sh',  
                '%{file.directory}/common.sh',  
                '%{file.directory}/bgfx_shader.sh'
            }
        filter { "system:windows", "configurations:Debug*" }
            buildoptions { "/fsanitize=address" }
            flags { "NoIncrementalLink" }
            editandcontinue "Off"
        filter "platforms:x86_64"
            libdirs { "lib/SDL2-2.0.8/lib/x64" }
        filter "platforms:x86"
            libdirs { "lib/SDL2-2.0.8/lib/x86" }
        filter "system:windows"
            links { "gdi32", "kernel32", "psapi" }
        filter "system:linux"
            links { "dl", "GL", "pthread", "X11" }
        filter "system:macosx"
            links { "QuartzCore.framework", "Metal.framework", "Cocoa.framework", "IOKit.framework", "CoreVideo.framework" }
            includedirs { "source/third_party/bx/include/compat/osx" }
            buildoptions { "-x objective-c++" }
        filter "action:vs*"
            defines "_CRT_SECURE_NO_WARNINGS"
            includedirs { "source/third_party/bx/include/compat/msvc" }
        filter { "system:windows", "action:gmake" }
            includedirs { "source/third_party/bx/include/compat/mingw" }