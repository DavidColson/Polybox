// Copyright 2020-2022 David Colson. All rights reserved.

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
#include "bind_mesh.h"
#include "bind_scene.h"
#include "buffer.h"
#include "cpu.h"
#include "lua_common.h"
#include "image.h"
#include "font.h"
#include "graphics.h"
#include "graphics_platform.h"
#include "input.h"
#include "mesh.h"
#include "rect_packing.h"
#include "scene.h"
#include "serialization.h"
#include "shapes.h"

// code
#include "asset_importer.cpp"
#include "bind_graphics.cpp"
#include "bind_input.cpp"
#include "bind_mesh.cpp"
#include "bind_scene.cpp"
#include "buffer.cpp"
#include "cpu.cpp"
#include "font.cpp"
#include "graphics.cpp"
#include "graphics_platform_d3d11.cpp"
#include "image.cpp"
#include "input.cpp"
#include "lua_common.cpp"
#include "mesh.cpp"
#include "rect_packing.cpp"
#include "scene.cpp"
#include "serialization.cpp"
#include "shapes.cpp"


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
    u64 frames = PlatformDebug::CollectStackTrace(trace, 100, 2);
    String stackTrace = PlatformDebug::PrintStackTraceToString(trace, frames, g_pArenaFrame);
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

int main(int argc, char* argv[]) {
	g_pArenaFrame = ArenaCreate();
	g_pArenaPermenant = ArenaCreate();

	Log::LogConfig log;
	log.critCrashes = false;
	log.customHandler1 = AssertHandler;
	Log::SetConfig(log);

	if (argc > 1) {
		if (strcmp(argv[1], "import") == 0) {
			if (argc < 4) {
				Log::Info("required format for import is \"import [-t|-b|-c|-bt] path/source_filename.file path/output_filename.file\"");
				Log::Info("-t: text     -b: binary uncompressed      -c: binary compressed  -bt: base64 encoded binary");
				Log::Info("will default to -c");
				return 1;
			}
			u8 format = 0x3;
			u8 fileArg = 3;
			if (strcmp(argv[2], "-t") == 0) {
				format = 0;
			}
			else if (strcmp(argv[2], "-b") == 0) {
				format = 0x1;
			}
			else if (strcmp(argv[2], "-c") == 0) {
				format = 0x3;
			}
			else if (strcmp(argv[2], "-bt") == 0) {
				format = 0x7;
			}
			else {
				fileArg = 2;
			}

			// TODO: if paths are given instead of files, process the whole folder instead
			Arena* pArena = ArenaCreate();
			i32 result = AssetImporter::Import(pArena, format, String(argv[fileArg]), String(argv[fileArg+1]));
			ArenaFinished(pArena);
			return result;
		}
		else {
			Log::Info("Supported commands are currently just \"-import\", enter it without args to get help");
			return 1;
		}
	}
	else {
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

		Cpu::CompileAndLoadProgram("systemroot/tank_demo/game.luau");
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

		Cpu::End();
		Shutdown();

	}
	i32 n = ReportMemoryLeaks();

	ArenaFinished(g_pArenaFrame);
	ArenaFinished(g_pArenaPermenant);
	return 0;
}
