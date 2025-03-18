// Copyright 2020-2022 David Colson. All rights reserved.

#define _CRT_SECURE_NO_WARNINGS
#define LZ4_DISABLE_DEPRECATE_WARNINGS

// platform
#pragma warning (disable : 5105)
#include "Windows.h"
#include "dbghelp.h"
#include "d3d11.h"
#include "dxgi.h"
#undef min
#undef max
#undef DrawText
#undef DrawTextEx
#undef DrawTextExA
#pragma comment(lib, "gdi32")
#pragma comment(lib, "kernel32")
#pragma comment(lib, "psapi")
#pragma comment(lib, "dbghelp")

// stdlib
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>

// common_lib
#include "common_lib.h"
#include "common_lib.cpp"

// lz4
#include "lz4.h"
#include "lz4.c"

// freetype
#include <ft2build.h>
#include <freetype/ftmm.h>
#include FT_FREETYPE_H
#pragma comment(lib, "freetype.lib")

// luau
#include <lua.h>
#include <lualib.h>
#include <luacode.h>
#include <Luau/Frontend.h>
#include <Luau/BuiltinDefinitions.h>
#pragma comment(lib, "Luau.Compiler.lib")
#pragma comment(lib, "Luau.Analysis.lib")
#pragma comment(lib, "Luau.VM.lib")
#pragma comment(lib, "Luau.Config.lib")
#pragma comment(lib, "Luau.Ast.lib")
#pragma comment(lib, "Luau.CLI.lib.lib")
#pragma comment(lib, "Luau.EqSat.lib")

// SDL
#include <SDL.h>
#include <SDL_syswm.h>
#pragma comment(lib, "SDL2main")
#pragma comment(lib, "SDL2")

// Sokol
#define SOKOL_GFX_IMPL
#define SOKOL_D3D11
#include <sokol_gfx.h>

// stbimage
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// includes
#include "asset_importer.h"
#include "bind_graphics.h"
#include "bind_input.h"
#include "userdata.h"
#include "cpu.h"
#include "graphics.h"
#include "graphics_platform.h"
#include "input.h"
#include "rect_packing.h"
#include "serialization.h"
#include "shapes.h"
#include "virtual_filesystem.h"

// code
#include "asset_importer.cpp"
#include "bind_graphics.cpp"
#include "bind_input.cpp"
#include "userdata.cpp"
#include "cpu.cpp"
#include "graphics.cpp"
#include "graphics_platform_d3d11.cpp"
#include "input.cpp"
#include "rect_packing.cpp"
#include "serialization.cpp"
#include "shapes.cpp"
#include "virtual_filesystem.cpp"


// ***********************************************************************

int ShowAssertDialog(String errorMsg) {
    const SDL_MessageBoxButtonData buttons[] = {
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Abort" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Debug" },
        { 0, 2, "Continue" },
    };

    StringBuilder builder(g_pArenaFrame);

    builder.Append(errorMsg);
    builder.Append("\n");
    builder.Append("Trace: \n");

    void* trace[100];
    u64 frames = Debug::CollectStackTrace(trace, 100, 2);
    String stackTrace = Debug::PrintStackTraceToString(trace, frames, g_pArenaFrame);
    builder.Append(stackTrace);

    String message = builder.CreateString(g_pArenaFrame);

    const SDL_MessageBoxData messageboxdata = {
        SDL_MESSAGEBOX_ERROR,
        NULL,
        "Error",
        message.pData,
        SDL_arraysize(buttons),
        buttons,
        nullptr
    };
    i32 buttonid;
    SDL_ShowMessageBox(&messageboxdata, &buttonid);
    return buttonid;
}

// ***********************************************************************

void AssertHandler(Log::LogLevel level, String message) {
    if (level <= Log::ECrit) {
        switch (ShowAssertDialog(message)) {
            case 0:
                _set_abort_behavior(0, _WRITE_ABORT_MSG);
                abort();
                break;
            case 1:
                __debugbreak();
                break;
            default:
                break;
        }
    }
}

// ***********************************************************************

static const String templateProject = R"TEMPLATE_PROJ(
function start()
	print("hello world")
end

function update()
    set_clear_color(0.25, 0.25, 0.25, 1.0)

	matrix_mode("Projection")
    identity()
    perspective(320, 240, 1, 20, 60)

    matrix_mode("View")
    identity()
    translate(-1.5, -1.5, -2)

	begin_object_3d("Triangles")
        color(1.0, 0.0, 0.0, 1.0)
        vertex(1.0, 1.0, 0.0)

        color(0, 1, 0, 1)
        vertex(1, 2, 0)

        color(0, 0, 1, 1)
        vertex(2.0, 2.0, 0.0)
    end_object_3d()
end

function close()
end
)TEMPLATE_PROJ";

// ***********************************************************************

void GenerateProject(String projectName) {
	// check if directory already exists for the project
	StringBuilder builder(g_pArenaFrame);
	builder.AppendFormat("system/%s/", projectName.pData);

	String projectPath = builder.CreateString(g_pArenaFrame, false);
	if (FolderExists(projectPath)) {
		Log::Warn("Project with name %s already exists", projectName.pData);
		return;
	}

	// create directory for project name
	MakeDirectory(projectPath, true);

	// create a main.lua template
	builder.Append("main.luau");
	WriteWholeFile(builder.CreateString(g_pArenaFrame), templateProject.pData, templateProject.length);

	Log::Info("Project '%s' created successfully", projectName.pData);
}

// ***********************************************************************

int main(int argc, char* argv[]) {
	g_pArenaFrame = ArenaCreate();
	g_pArenaPermenant = ArenaCreate();

	Log::LogConfig log;
	log.critCrashes = false;
	log.customHandler1 = AssertHandler;
	Log::SetConfig(log);

	String startupAppName;

	if (argc > 1) {
		if (strcmp(argv[1], "-import") == 0) {
			if (argc < 4) {
				Log::Info("required format for import is \"import [-t|-b|-c|-bt] path/source_filename <projectname>/output_filename.file\"");
				Log::Info("-t: text     -b: binary uncompressed      -c: binary compressed  -bt: base64 encoded binary");
				Log::Info("will default to -c");
				return 1;
			}
			u8 format = 0x3;
			u8 fileArg = 3;
			if (strcmp(argv[2], "-t") == 0) {
				format = 0;
			}
			else if (strcmp(argv[2], "-bt") == 0) {
				format = 0x7;
			}
			else if (strcmp(argv[2], "-b") == 0) {
				format = 0x1;
			}
			else if (strcmp(argv[2], "-c") == 0) {
				format = 0x3;
			}
			else {
				fileArg = 2;
			}

			// @TODO: if paths are given instead of files, process the whole folder instead
			Arena* pArena = ArenaCreate();
			i32 result = AssetImporter::Import(pArena, format, String(argv[fileArg]), String(argv[fileArg+1]));
			if (result >= 0) 
				Log::Info("Import succeeded");
			else 
				Log::Info("Import failed");

			ArenaFinished(pArena);
			return result;
		}
		else if (strcmp(argv[1], "-new") == 0) {
			GenerateProject(String(argv[2]));
			return 1;
		}
		else if (strcmp(argv[1], "-start") == 0) {
			startupAppName = String(argv[2]);
		}
		else {
			Log::Info("Valid commands are:");
			Log::Info(" ");
			Log::Info("-start [project_name]:");
			Log::Info(" ");
			Log::Info("	Start polybox booting directly into the named app");
			Log::Info(" ");
			Log::Info("-new [project_name]:");
			Log::Info(" ");
			Log::Info("	Create a new project with the given name");
			Log::Info(" ");
			Log::Info("-import [-t|-b|-c|-bt] path/source.file path/output.file");
			Log::Info(" ");
			Log::Info("	Import a raw asset into the polybox formats");
			Log::Info("	-t: text     -b: binary uncompressed      -c: binary compressed  -bt: base64 encoded binary");
			Log::Info("	will default to -c");
			return 1;
		}
	}


	i32 winWidth = 1280;
	i32 winHeight = 720;

	SDL_Window* pWindow = SDL_CreateWindow(
		"Polybox",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		i32(winWidth),
		i32(winHeight),
		SDL_WINDOW_RESIZABLE);

	Cpu::Init();
	GraphicsInit(pWindow, winWidth, winHeight);
	InputInit();

	Cpu::LoadApp(startupAppName);
	Cpu::Start();

	bool gameRunning = true;
	f32 deltaTime = 0.016f;
	Vec2i relativeMouseStartLocation { Vec2i(0, 0) };
	bool isCapturingMouse = false;
	while (gameRunning) {
		Uint64 frameStart = SDL_GetPerformanceCounter();

		ClearStates();
		// Deal with events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ProcessEvent(&event);
			switch (event.type) {
				case SDL_KEYDOWN: {
					if (event.key.keysym.scancode == SDL_SCANCODE_TAB && event.key.keysym.mod & KMOD_LSHIFT) {
						isCapturingMouse = !isCapturingMouse;
						if (isCapturingMouse) {
							SDL_GetGlobalMouseState(&relativeMouseStartLocation.x, &relativeMouseStartLocation.y);
							SDL_SetRelativeMouseMode(SDL_TRUE);
						} else {
							SDL_SetRelativeMouseMode(SDL_FALSE);
							SDL_WarpMouseGlobal(relativeMouseStartLocation.x, relativeMouseStartLocation.y);
						}
					}
				} break;
				case SDL_WINDOWEVENT:
					switch (event.window.event) {
						case SDL_WINDOWEVENT_CLOSE:
							gameRunning = false;
						break;
						default:
						break;
					}
				break;
				case SDL_QUIT:
					gameRunning = false;
				break;
			}
		}
		UpdateInputs(deltaTime, Vec2f(320.0f, 240.0f), Vec2f((f32)winWidth, (f32)winHeight));

		Cpu::Tick(deltaTime);

		DrawFrame(winWidth, winHeight);

		deltaTime = f32(SDL_GetPerformanceCounter() - frameStart) / SDL_GetPerformanceFrequency();

		ArenaReset(g_pArenaFrame);
	}

	Cpu::Close();
	Shutdown();

	i32 n = ReportMemoryLeaks();

	ArenaFinished(g_pArenaFrame);
	ArenaFinished(g_pArenaPermenant);
	return 0;
}
