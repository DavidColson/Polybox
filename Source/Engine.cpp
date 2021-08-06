// Copyright 2020-2021 David Colson. All rights reserved.

#include "Engine.h"

#include "Core/Vec3.h"
#include "Core/Matrix.h"

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "GraphicsChip.h"

// This defines a macro called min somehow? We should avoid it at all costs and include it last
#include <SDL_syswm.h>

#include <format>

#include <SDL.h>

int main(int argc, char *argv[])
{
	{
		std::string hello = std::format("{} {}!", "hello", "world");

		int winWidth = 1600;
		int winHeight = 900;

		SDL_Window* pWindow = SDL_CreateWindow(
			"Polybox",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			int(winWidth),
			int(winHeight),
			SDL_WINDOW_RESIZABLE
		);

		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(pWindow, &wmInfo);
		HWND hwnd = wmInfo.info.win.window;

		bgfx::Init init;
		init.type = bgfx::RendererType::Direct3D11;
		init.platformData.ndt = NULL;
		init.platformData.nwh = wmInfo.info.win.window;

		bgfx::renderFrame();

		bgfx::init(init);
		const bgfx::ViewId kClearView = 0;
		bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x404040ff, 1.0f, 0);


		bgfx::setViewRect(kClearView, 0, 0, winWidth, winHeight);
		bgfx::reset(winWidth, winHeight, BGFX_RESET_VSYNC);

		GraphicsChip gpu = GraphicsChip();
		gpu.Init();

		bool gameRunning = true;
		float deltaTime = 0.016f;
	    Vec2i relativeMouseStartLocation{ Vec2i(0, 0) };
		bool isCapturingMouse = false;
		while (gameRunning)
		{
			Uint64 frameStart = SDL_GetPerformanceCounter();

			// Deal with events
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				case SDL_KEYDOWN:
					{
						if (event.key.keysym.scancode == SDL_SCANCODE_TAB && event.key.keysym.mod & KMOD_LSHIFT)
						{
							isCapturingMouse = !isCapturingMouse;
							if (isCapturingMouse)
							{
								SDL_GetGlobalMouseState(&relativeMouseStartLocation.x, &relativeMouseStartLocation.y);
								SDL_SetRelativeMouseMode(SDL_TRUE);
							}
							else
							{
								SDL_SetRelativeMouseMode(SDL_FALSE);
								SDL_WarpMouseGlobal(relativeMouseStartLocation.x, relativeMouseStartLocation.y);
							}
						}
					}
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
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
			bgfx::touch(kClearView);

			gpu.BeginObject(PrimitiveType::LineStrip);

			gpu.Vertex(Vec3f(0.1f, 0.1f, 0.0f));
			gpu.Vertex(Vec3f(0.1f, 0.2f, 0.0f));
			gpu.Vertex(Vec3f(0.2f, 0.2f, 0.0f));

			gpu.EndObject();

			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(10, 10, 0x0f, "Hello world");
			bgfx::setDebug(BGFX_DEBUG_TEXT);
			bgfx::frame();

			deltaTime = float(SDL_GetPerformanceCounter() - frameStart) / SDL_GetPerformanceFrequency();
		}
	}
	bgfx::shutdown();

	return 0;
}