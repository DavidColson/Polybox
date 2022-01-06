// Copyright 2020-2021 David Colson. All rights reserved.

#include "Engine.h"

#include "Core/Vec3.h"
#include "Core/Matrix.h"
#include "GraphicsChip.h"
#include "Shapes.h"
#include "Model.h"
#include "Font.h"
#include "Image.h"
#include "Bind_GraphicsChip.h"

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <format>
#include <SDL.h>
#include <SDL_syswm.h>
#undef DrawText
#undef DrawTextEx

void RecursiveTransformTree(Node* pTreeBase)
{
	for (Node* pChild : pTreeBase->m_children)
	{
		Matrixf childLocalTrans = Matrixf::MakeTQS(pChild->m_translation, pChild->m_rotation, pChild->m_scale);
		pChild->m_worldTransform = pTreeBase->m_worldTransform * childLocalTrans;

		if (!pChild->m_children.empty())
			RecursiveTransformTree(pChild);
	}
}

void TransformNodeHeirarchy(std::vector<Node>& nodeList)
{
	std::vector<Node*> rootTransforms;

	for (Node& node : nodeList)
	{
		if (node.m_pParent == nullptr)
		{
			node.m_worldTransform = Matrixf::MakeTQS(node.m_translation, node.m_rotation, node.m_scale);

			if (!node.m_children.empty())
				rootTransforms.push_back(&node);
		}
	}

	for (Node* pRoot : rootTransforms)
	{
		RecursiveTransformTree(pRoot);
	}	
}

int main(int argc, char *argv[])
{
	{
		std::string hello = std::format("{} {}!", "hello", "world");

		int winWidth = 1280;
		int winHeight = 960;

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
		bgfx::reset(winWidth, winHeight, BGFX_RESET_VSYNC);

		GraphicsChip gpu = GraphicsChip();
		gpu.Init();

		Scene tankScene("Assets/tank.gltf");
		TransformNodeHeirarchy(tankScene.m_nodes);

		// Lua embedding experiments
		lua_State* pLua = luaL_newstate();
		luaL_openlibs(pLua); // Do we want to expose normal lua libs? Maybe not, pico doesn't, also have the option to open just some of the libs

		Bind::BindGraphicsChip(pLua, &gpu);

		if (luaL_dofile(pLua, "Assets/game.lua") != LUA_OK)
		{
			std::string format = std::format("Lua Runtime Error: {}", lua_tostring(pLua, lua_gettop(pLua)));
			OutputDebugStringA(format.c_str());
		}

		lua_getglobal(pLua, "Start");
		if (lua_isfunction(pLua, -1))
		{
			if (lua_pcall(pLua, 0, 0, 0) != LUA_OK)
			{
				std::string format = std::format("Lua Runtime Error: {}", lua_tostring(pLua, lua_gettop(pLua)));
				OutputDebugStringA(format.c_str());
			}
		}

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
			bgfx::touch(0);

			// Lua updates

			lua_getglobal(pLua, "Update");
			if (lua_isfunction(pLua, -1))
			{
				if (lua_pcall(pLua, 0, 0, 0) != LUA_OK)
				{
					std::string format = std::format("Lua Runtime Error: {}", lua_tostring(pLua, lua_gettop(pLua)));
					OutputDebugStringA(format.c_str());
					__debugbreak();
				}
			}
			
			gpu.DrawFrame((float)winWidth, (float)winHeight);

			//bgfx::setDebug(BGFX_DEBUG_STATS);
			bgfx::frame();

			deltaTime = float(SDL_GetPerformanceCounter() - frameStart) / SDL_GetPerformanceFrequency();
		}

		lua_getglobal(pLua, "End");
		if (lua_isfunction(pLua, -1))
		{
			if (lua_pcall(pLua, 0, 0, 0) != LUA_OK)
			{
				std::string format = std::format("Lua Runtime Error: {}", lua_tostring(pLua, lua_gettop(pLua)));
				OutputDebugStringA(format.c_str());
			}
		}

		lua_close(pLua);
	}
	bgfx::shutdown();

	return 0;
}