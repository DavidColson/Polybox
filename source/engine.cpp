// Copyright 2020-2022 David Colson. All rights reserved.

#include "engine.h"

#include "bind_game_chip.h"
#include "bind_graphics_chip.h"
#include "bind_mesh.h"
#include "bind_scene.h"
#include "font.h"
#include "game_chip.h"
#include "graphics_chip_sokol.h"
#include "image.h"
#include "mesh.h"
#include "scene.h"
#include "shapes.h"

#include "lua.h"
#include "luacode.h"
#include "Luau/Frontend.h"
#include "Luau/BuiltinDefinitions.h"

#include <SDL.h>
#include <sokol_gfx.h>
#include <defer.h>
#include <log.h>
#include <matrix.h>
#include <platform_debug.h>
#include <vec3.h>
#include <hashmap.inl>
#include <resizable_array.inl>
#undef DrawText
#undef DrawTextEx


// ***********************************************************************

int ShowAssertDialog(String errorMsg) {
    const SDL_MessageBoxButtonData buttons[] = {
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Abort" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Debug" },
        { 0, 2, "Continue" },
    };

    StringBuilder builder;

    builder.Append(errorMsg);
    builder.Append("\n");
    builder.Append("Trace: \n");

    void* trace[100];
    usize frames = PlatformDebug::CollectStackTrace(trace, 100, 2);
    String stackTrace = PlatformDebug::PrintStackTraceToString(trace, frames);
    builder.Append(stackTrace);

    String message = builder.CreateString();
    defer(FreeString(message));

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


static void* limitedRealloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    if (nsize == 0)
    {
        free(ptr);
        return nullptr;
    }
    else if (nsize > 8 * 1024 * 1024)
    {
        // For testing purposes return null for large allocations so we can generate errors related to memory allocation failures
        return nullptr;
    }
    else
    {
        return realloc(ptr, nsize);
    }
}

struct LuaFileResolver : Luau::FileResolver
{
    std::optional<Luau::SourceCode> readSource(const Luau::ModuleName& name) override
    {
        Luau::SourceCode::Type sourceType;
        std::optional<std::string> source = std::nullopt;

        // If the module name is "-", then read source from stdin
        if (name == "-")
        {
			Assert("Not sure what this means, come back fix this");
        }
        else
        {

			SDL_RWops* pFileRead = SDL_RWFromFile(name.c_str(), "rb");
			u64 sourceSize = SDL_RWsize(pFileRead);
			std::string result(sourceSize, 0);
			SDL_RWread(pFileRead, result.data(), sourceSize, 1);
			SDL_RWclose(pFileRead);

			source = result;
			sourceType = Luau::SourceCode::Module;
        }

        if (!source)
            return std::nullopt;

        return Luau::SourceCode{*source, sourceType};
    }

    std::optional<Luau::ModuleInfo> resolveModule(const Luau::ModuleInfo* context, Luau::AstExpr* node) override
    {
        if (Luau::AstExprConstantString* expr = node->as<Luau::AstExprConstantString>())
        {
            Luau::ModuleName name = std::string(expr->value.data, expr->value.size) + ".luau";
			SDL_RWops* pFileRead = SDL_RWFromFile(name.c_str(), "rb");
            if (pFileRead == nullptr)
            {
                // fall back to .lua if a module with .luau doesn't exist
                name = std::string(expr->value.data, expr->value.size) + ".lua";
            }

            return {{name}};
        }

        return std::nullopt;
    }

    std::string getHumanReadableModuleName(const Luau::ModuleName& name) const override
    {
        if (name == "-")
            return "stdin";
        return name;
    }
};
// ***********************************************************************

int main(int argc, char* argv[]) {
	{
		Log::LogConfig log;
		log.critCrashes = false;
		log.customHandler1 = AssertHandler;
		Log::SetConfig(log);

		i32 winWidth = 1280;
		i32 winHeight = 960;

		SDL_Window* pWindow = SDL_CreateWindow(
			"Polybox",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			i32(winWidth),
			i32(winHeight),
			SDL_WINDOW_RESIZABLE);

		GraphicsInit(pWindow, winWidth, winHeight);

		GameChip game = GameChip();
		game.Init();

		lua_State* pLua = lua_newstate(limitedRealloc, nullptr);
		// luaL_openlibs(pLua);  // Do we want to expose normal lua libs? Maybe not, pico doesn't, also have the option to open just some of the libs

		Bind::BindGraphicsChip(pLua);
		Bind::BindMesh(pLua);
		Bind::BindScene(pLua);
		Bind::BindGameChip(pLua, &game);

		// Prepare frontend for typechecking
		bool wasError = false;
		{
			Luau::FrontendOptions frontendOptions;
			frontendOptions.runLintChecks = true;

			LuaFileResolver fileResolver;
			Luau::NullConfigResolver configResolver;
			Luau::Frontend frontend(&fileResolver, &configResolver, frontendOptions);

			Luau::registerBuiltinGlobals(frontend, frontend.globals);
			Luau::freeze(frontend.globals.globalTypes); // Marks the type memory as read only

			// Add our global type definitions

			// give errors on parsing the type definitions

			// run check
			Luau::CheckResult result = frontend.check("assets/game.lua");

			if (!result.errors.empty())
				wasError = true;

			// print errors and lint warnings
			for (Luau::TypeError& error : result.errors) {
				std::string humanReadableName = frontend.fileResolver->getHumanReadableModuleName(error.moduleName);
				if (const Luau::SyntaxError* syntaxError = Luau::get_if<Luau::SyntaxError>(&error.data))
					Log::Warn("SyntaxError in [%s:%d] - %s", humanReadableName.c_str(), error.location.begin.line + 1, syntaxError->message.c_str());
				else
					Log::Warn("TypeError in [%s:%d] - %s", humanReadableName.c_str(), error.location.begin.line + 1, Luau::toString(error, Luau::TypeErrorToStringOptions{frontend.fileResolver}).c_str());
			}

			for (Luau::LintWarning& error : result.lintResult.errors) {
				std::string humanReadableName = "assets/game.lua";
				Log::Warn("LintError in [%s:%d] (%s) - %s", humanReadableName.c_str(), error.location.begin.line + 1,
					  Luau::LintWarning::getName(error.code), error.text.c_str());
			}
			
			for (Luau::LintWarning& error : result.lintResult.warnings) {
				std::string humanReadableName = "assets/game.lua";
				Log::Warn("LintWarning in [%s:%d] (%s) - %s", humanReadableName.c_str(), error.location.begin.line + 1,
					  Luau::LintWarning::getName(error.code), error.text.c_str());
			}
		}

		if (wasError == false) {
			SDL_RWops* pFileRead = SDL_RWFromFile("assets/game.lua", "rb");
			u64 sourceSize = SDL_RWsize(pFileRead);
			char* pSource = (char*)g_Allocator.Allocate(sourceSize * sizeof(char));
			SDL_RWread(pFileRead, pSource, sourceSize, 1);
			pSource[sourceSize] = '\0';
			SDL_RWclose(pFileRead);

			usize bytecodeSize = 0;
			char* pBytecode = luau_compile(pSource, sourceSize, nullptr, &bytecodeSize);
			int result = luau_load(pLua, "assets/game.lua", pBytecode, bytecodeSize, 0);

			if (result != 0) {
				Log::Warn("Lua Load Error: %s", lua_tostring(pLua, lua_gettop(pLua)));
				lua_pop(pLua, 1);
			}

			if (lua_pcall(pLua, 0, 0, 0) != LUA_OK) {
				Log::Warn("Lua Runtime Error: %s", lua_tostring(pLua, lua_gettop(pLua)));
				lua_pop(pLua, 1);
			}
		}

		lua_getglobal(pLua, "Start");
		if (lua_isfunction(pLua, -1)) {
			if (lua_pcall(pLua, 0, 0, 0) != LUA_OK) {
				Log::Warn("Lua Runtime Error: %s", lua_tostring(pLua, lua_gettop(pLua)));
				lua_pop(pLua, 1);
			}
		}

		bool gameRunning = true;
		f32 deltaTime = 0.016f;
		Vec2i relativeMouseStartLocation { Vec2i(0, 0) };
		bool isCapturingMouse = false;
		while (gameRunning) {
			Uint64 frameStart = SDL_GetPerformanceCounter();

			game.ClearStates();
			// Deal with events
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				game.ProcessEvent(&event);
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
			game.UpdateInputs(deltaTime, Vec2f(320.0f, 240.0f), Vec2f((f32)winWidth, (f32)winHeight));

			// Lua updates

			lua_getglobal(pLua, "Update");
			if (lua_isfunction(pLua, -1)) {
				lua_pushnumber(pLua, deltaTime);
				if (lua_pcall(pLua, 1, 0, 0) != LUA_OK) {
					Log::Warn("Lua Runtime Error: %s", lua_tostring(pLua, lua_gettop(pLua)));
					lua_pop(pLua, 1);
				}
			} else {
				lua_pop(pLua, 1);
			}

			DrawFrame(winWidth, winHeight);

			deltaTime = f32(SDL_GetPerformanceCounter() - frameStart) / SDL_GetPerformanceFrequency();
		}

		lua_getglobal(pLua, "End");
		if (lua_isfunction(pLua, -1)) {
			if (lua_pcall(pLua, 0, 0, 0) != LUA_OK) {
				Log::Warn("Lua Runtime Error: %s", lua_tostring(pLua, lua_gettop(pLua)));
			}
		}

		lua_close(pLua);
		game.Shutdown();
	}
	i32 n = ReportMemoryLeaks();
	Log::Info("Memory Leak Reports %i leaks", n);

	return 0;
}
