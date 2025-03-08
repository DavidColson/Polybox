@echo off
setlocal enabledelayedexpansion

:: unpack arguments
for %%a in (%*) do set "%%a=1"

if not "%release%"=="1" set debug=1
if "%debug%"=="1"     set cmake_config=Debug
if "%release%"=="1"   set cmake_config=Release

if not "%build_luau%"=="1" set build_luau=0 

:: build shaders
echo ----- Compiling Shaders ------
echo ------------------------------
set shader_cl=source\third_party\sokol-tools\bin\win32\sokol-shdc.exe
%shader_cl% --input shaders\core3d.shader --output source\generated\core3d.h --slang hlsl5 --bytecode --errfmt msvc
%shader_cl% --input shaders\compositor.shader --output source\generated\compositor.h --slang hlsl5 --bytecode --errfmt msvc

:: include directories
set cl_includes= /I ..\source\
set cl_includes= !cl_includes! /I ..\source\generated\ 
set cl_includes= !cl_includes! /I ..\source\common_lib\source\ 
set cl_includes= !cl_includes! /I ..\source\third_party\sokol\
set cl_includes= !cl_includes! /I ..\source\third_party\FreeType\include\
set cl_includes= !cl_includes! /I ..\source\third_party\luau\Compiler\include\
set cl_includes= !cl_includes! /I ..\source\third_party\luau\Analysis\include\
set cl_includes= !cl_includes! /I ..\source\third_party\luau\VM\include\
set cl_includes= !cl_includes! /I ..\source\third_party\luau\Config\include\
set cl_includes= !cl_includes! /I ..\source\third_party\luau\Ast\include\
set cl_includes= !cl_includes! /I ..\source\third_party\luau\Common\include\
set cl_includes= !cl_includes! /I ..\source\third_party\stb\
set cl_includes= !cl_includes! /I ..\source\third_party\lz4\
set cl_includes= !cl_includes! /I ..\source\third_party\SDL2\include\
set cl_libs= /libpath:..\source\third_party\SDL2\lib\x64\
set cl_libs= !cl_libs! /libpath:..\source\third_party\luau\cmake\%cmake_config%\

:: compile/link options
set cl_common= %cl_includes% /Zc:preprocessor /std:c++20 /Bt /GR- /W3 /WX
set cl_debug= call cl /Od /Ob0 /Zi /MDd /fsanitize=address %cl_common%
set cl_release= call cl /O2 /Ob2 /MD /DNDEBUG %cl_common%
set cl_link= /link /incremental:no /subsystem:console %cl_libs% /natvis:..\source\third_party\luau\tools\natvis\VM.natvis
if "%debug%"=="1"     set compile=%cl_debug%
if "%release%"=="1"   set compile=%cl_release%

:: prep build directory
if not exist build mkdir build

:: compile luau
if not exist source\third_party\luau\cmake\%cmake_config%\Luau.VM.lib set build_luau=1
if not exist source\third_party\luau\cmake\%cmake_config%\Luau.Compiler.lib set build_luau=1
if not exist source\third_party\luau\cmake\%cmake_config%\Luau.Analysis.lib set build_luau=1
if %build_luau%==1 (
	echo ------- Compiling Luau -------
	echo ------------------------------
	pushd source\third_party\luau
	if not exist cmake mkdir cmake
	pushd cmake
	cmake .. -DCMAKE_BUILD_TYPE=%cmake_config%
	cmake --build . --target Luau.Compiler --config %cmake_config%
	cmake --build . --target Luau.VM --config %cmake_config%
	cmake --build . --target Luau.Analysis --config %cmake_config%
	popd
	popd
)

:: compile freetype
pushd build
set freetype_exist=1
if not exist freetype.lib set freetype_exist=0
if %freetype_exist%==0 (
	echo ----- Compiling Freetype -----
	echo ------------------------------
	%compile% /c ..\source\third_party\freetype\freetype_all.c
	call lib /out:freetype.lib freetype_all.obj
)
popd

:: compile Polybox
echo ----- Compiling Polybox ------
echo ------------------------------
pushd build
%compile% ..\source\main.cpp %cl_link% /out:polybox.exe
popd
