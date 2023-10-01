#include "compiler_explorer.h"

#include <resizable_array.inl>
#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <backends/imgui_impl_bgfx.h>
#include <imgui.h>

// This defines a macro called min somehow? We should avoid it at all costs and include it last
namespace SDL {
	#include <SDL_syswm.h>
}

void RunCompilerExplorer() {
	bool gameRunning{ true };
	float deltaTime;
	uint64_t frameStartTime;
	const bgfx::ViewId kClearView{ 255 };
	int width {2000};
	int height {1200};

	// Init graphics, window, imgui etc
	SDL_Window* pWindow = SDL_CreateWindow(
		"Compiler Explorer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		SDL_WINDOW_RESIZABLE
	);

	SDL::SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL::SDL_GetWindowWMInfo(pWindow, &wmInfo);
	SDL::HWND hwnd = wmInfo.info.win.window;

	bgfx::Init init;
	init.type = bgfx::RendererType::Direct3D11;
	init.platformData.ndt = NULL;
	init.platformData.nwh = wmInfo.info.win.window;

	bgfx::renderFrame();

	bgfx::init(init);
	bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x404040ff, 1.0f, 0);
	bgfx::setViewRect(kClearView, 0, 0, width, height);
	bgfx::reset(width, height, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X8);
	gameRunning = true;
	deltaTime = 0.016f;

	ImGuiBackend::Initialize(pWindow);

	while (gameRunning) {
		frameStartTime = SDL_GetPerformanceCounter();
		// Deal with events
		SDL_Event event;
		while (SDL_PollEvent(&event)){
			ImGuiBackend::ProcessEvent(event);
			
			switch (event.type) {
				case SDL_WINDOWEVENT:
					switch (event.window.event) {
						case SDL_WINDOWEVENT_RESIZED:
							// Update the bgfx framebuffer to match the new window size
							bgfx::reset(event.window.data1, event.window.data2, BGFX_RESET_VSYNC);
							break;
						default:
							break;
					}
					break;
				case SDL_QUIT:
					gameRunning = false;
					break;
				default:
					break;
			}
		}

		ImGuiBackend::NewFrame();

		ImGui::ShowDemoWindow();

		// Do your rendering here

		// [] Render the code in a window
		// [] Render the ast as an explorable tree with a properties panel

		ImGuiBackend::Render();
		bgfx::touch(kClearView);

		bgfx::frame();
		float targetFrameTime = 0.0166f;
		float realframeTime = float(SDL_GetPerformanceCounter() - frameStartTime) / SDL_GetPerformanceFrequency();
		if (realframeTime < targetFrameTime)
		{
			unsigned int waitTime = int((targetFrameTime - realframeTime) * 1000.0);
			SDL_Delay(waitTime);
		}

		deltaTime = float(SDL_GetPerformanceCounter() - frameStartTime) / SDL_GetPerformanceFrequency();
	}

	ImGuiBackend::Destroy();
	bgfx::shutdown();
}