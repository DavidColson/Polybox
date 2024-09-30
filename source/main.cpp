// Copyright 2020-2022 David Colson. All rights reserved.

#include "main.h"

#include "input.h"
#include "graphics.h"
#include "cpu.h"
#include "asset_importer.h"

#include <SDL.h>
#include <defer.h>
#include <log.h>
#include <platform_debug.h>
#include <string_builder.h>
#include <light_string.h>
#include <memory_tracker.h>
#include <string.h>

#include <stdlib.h>
#undef DrawText
#undef DrawTextEx



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
    usize frames = PlatformDebug::CollectStackTrace(trace, 100, 2);
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
		if (strcmp(argv[1], "-import") == 0) {
			if (argc != 4) {
				Log::Info("required format for import is \"-import path/source_filename.file path/output_path\"");
				return 1;
			}

			return AssetImporter::Import(String(argv[2]), String(argv[3]));
		}
		else {
			Log::Info("Supported commands are currently just \"-import\", enter it without args to get help");
			return 1;
		}
	}
	else {
		i32 winWidth = 1280;
		i32 winHeight = 960;

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
