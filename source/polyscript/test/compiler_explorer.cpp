#include "compiler_explorer.h"

#include <resizable_array.inl>
#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <bgfx/embedded_shader.h>
#include <bx/allocator.h>
#include <bx/timer.h>
#include <bx/math.h>

// This defines a macro called min somehow? We should avoid it at all costs and include it last
namespace SDL {
	#include <SDL_syswm.h>
}

#include "imgui_data/vs_ocornut_imgui.bin.h"
#include "imgui_data/fs_ocornut_imgui.bin.h"
#include "imgui_data/vs_imgui_image.bin.h"
#include "imgui_data/fs_imgui_image.bin.h"

#include "imgui_data/roboto_regular.ttf.h"
#include "imgui_data/robotomono_regular.ttf.h"

#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

// ***********************************************************************

static const bgfx::EmbeddedShader s_embeddedShaders[] =
{
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(vs_imgui_image),
    BGFX_EMBEDDED_SHADER(fs_imgui_image),

    BGFX_EMBEDDED_SHADER_END()
};

// ***********************************************************************

struct BackendData
{
    bgfx::ShaderHandle imguiVertex;
    bgfx::ShaderHandle imguiFragment;
    bgfx::ShaderHandle imguiImageVertex;
    bgfx::ShaderHandle imguiImageFragment;

    SDL_Window* m_SdlWindow;
    bgfx::VertexLayout  m_layout;
    bgfx::ProgramHandle m_program;
    bgfx::ProgramHandle m_imageProgram;
    bgfx::TextureHandle m_texture;
    bgfx::UniformHandle s_tex;
    bgfx::UniformHandle u_imageLodEnabled;
    
    bgfx::ViewId m_mainViewId;
    ResizableArray<bgfx::ViewId> m_freeViewIds;
    bgfx::ViewId m_subViewId = 100;

    bgfx::ViewId AllocateViewId() {
        if (m_freeViewIds.count != 0) {
            const bgfx::ViewId id = m_freeViewIds[m_freeViewIds.count - 1];
            m_freeViewIds.PopBack();
            return id;
        }
        return m_subViewId++;
    }

    void FreeViewId(bgfx::ViewId id) {
        m_freeViewIds.PushBack(id);
    }
};

// ***********************************************************************

struct ViewportData 
{
    bgfx::FrameBufferHandle frameBufferHandle;
    bgfx::ViewId viewId = 0;
    uint16_t width = 0;
    uint16_t height = 0;
};

// ***********************************************************************

inline bool CheckAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout& _layout, uint32_t _numIndices)
{
    return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout)
        && (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices) )
        ;
}

// ***********************************************************************

void* GetNativeWindowHandle(SDL_Window* pWindow) {
    SDL::SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(pWindow, &wmi)) {
        return NULL;
    }
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#if ENTRY_CONFIG_USE_WAYLAND
    wl_egl_window* win_impl = (wl_egl_window*)SDL_GetWindowData(pWindow, "wl_egl_window");
    if (!win_impl) {
        int width, height;
        SDL_GetWindowSize(pWindow, &width, &height);
        struct wl_surface* surface = wmi.info.wl.surface;
        if (!surface)
            return nullptr;
        win_impl = wl_egl_window_create(surface, width, height);
        SDL_SetWindowData(pWindow, "wl_egl_window", win_impl);
    }
    return (void*)(uintptr_t)win_impl;
#else
    return (void*)wmi.info.x11.window;
#endif
#elif BX_PLATFORM_OSX
    return wmi.info.cocoa.window;
#elif BX_PLATFORM_WINDOWS
    return wmi.info.win.window;
#endif // BX_PLATFORM_
}

// ***********************************************************************

void OnCreateWindow(ImGuiViewport* pViewport) 
{
    ImGuiIO& io = ImGui::GetIO();
    BackendData* pBackend = (BackendData*)io.BackendRendererUserData;

    ViewportData* pData = new ViewportData();
    pViewport->RendererUserData = pData;
    // Setup view id and size
    pData->viewId = pBackend->AllocateViewId();
    pData->width = bx::max<uint16_t>((uint16_t)pViewport->Size.x, 1);
    pData->height = bx::max<uint16_t>((uint16_t)pViewport->Size.y, 1);
    // Create frame buffer
    pData->frameBufferHandle = bgfx::createFrameBuffer(GetNativeWindowHandle((SDL_Window*)pViewport->PlatformHandle), uint16_t(pData->width * pViewport->DrawData->FramebufferScale.x), uint16_t(pData->height * pViewport->DrawData->FramebufferScale.y));
    // Set frame buffer
    bgfx::setViewFrameBuffer(pData->viewId, pData->frameBufferHandle);
}

// ***********************************************************************

void OnDestroyWindow(ImGuiViewport* pViewport)
{
    ImGuiIO& io = ImGui::GetIO();
    BackendData* pBackend = (BackendData*)io.BackendRendererUserData;

    if (ViewportData* pData = (ViewportData*)pViewport->RendererUserData; pData)
    {
        pViewport->RendererUserData = nullptr;
        pBackend->FreeViewId(pData->viewId);
        bgfx::destroy(pData->frameBufferHandle);
        pData->frameBufferHandle.idx = bgfx::kInvalidHandle;
        delete pData;
    }
}

// ***********************************************************************

void OnSetWindowSize(ImGuiViewport* pViewport, ImVec2 size) {
    OnDestroyWindow(pViewport);
    OnCreateWindow(pViewport);
}

// ***********************************************************************

void RenderView(const bgfx::ViewId viewId, ImDrawData* pDrawData, uint32_t clearColor)
{
    ImGuiIO& io = ImGui::GetIO();
    BackendData* pBackend = (BackendData*)io.BackendRendererUserData;

    if (io.DisplaySize.x <= 0 || io.DisplaySize.y <= 0)
    {
        return;
    }

    if (clearColor) {
        bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
    }
    bgfx::touch(viewId);
    bgfx::setViewName(viewId, "ImGui");
    bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);
    
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(pDrawData->DisplaySize.x * pDrawData->FramebufferScale.x);
    int fb_height = (int)(pDrawData->DisplaySize.y * pDrawData->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    bgfx::setViewName(viewId, "ImGui");
    bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);

    const bgfx::Caps* caps = bgfx::getCaps();
    {
        float ortho[16];
        float x = pDrawData->DisplayPos.x;
        float y = pDrawData->DisplayPos.y;
        float width = pDrawData->DisplaySize.x;
        float height = pDrawData->DisplaySize.y;

        bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
        bgfx::setViewTransform(viewId, NULL, ortho);
        bgfx::setViewRect(viewId, 0, 0, uint16_t(width), uint16_t(height) );
    }

    const ImVec2 clipPos   = pDrawData->DisplayPos;       // (0,0) unless using multi-viewports
    const ImVec2 clipScale = pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int32_t ii = 0, num = pDrawData->CmdListsCount; ii < num; ++ii)
    {
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;

        const ImDrawList* drawList = pDrawData->CmdLists[ii];
        uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
        uint32_t numIndices  = (uint32_t)drawList->IdxBuffer.size();

        if (!CheckAvailTransientBuffers(numVertices, pBackend->m_layout, numIndices) )
        {
            // not enough space in transient buffer just quit drawing the rest...
            break;
        }

        bgfx::allocTransientVertexBuffer(&tvb, numVertices, pBackend->m_layout);
        bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

        ImDrawVert* verts = (ImDrawVert*)tvb.data;
        bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert) );

        ImDrawIdx* indices = (ImDrawIdx*)tib.data;
        bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx) );

        bgfx::Encoder* encoder = bgfx::begin();

        for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd)
        {
            if (cmd->UserCallback)
            {
                cmd->UserCallback(drawList, cmd);
            }
            else if (0 != cmd->ElemCount)
            {
                uint64_t state = 0
                    | BGFX_STATE_WRITE_RGB
                    | BGFX_STATE_WRITE_A
                    | BGFX_STATE_MSAA
                    | BGFX_STATE_BLEND_ALPHA
                    ;

                bgfx::TextureHandle th = pBackend->m_texture;
                bgfx::ProgramHandle program = pBackend->m_program;

                if (NULL != cmd->TextureId)
                {
                    union { ImTextureID ptr; struct { bgfx::TextureHandle handle; uint8_t flags; uint8_t mip; } s; } texture = { cmd->TextureId };
                    state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
                        ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
                        : BGFX_STATE_NONE
                        ;
                    th = texture.s.handle;
                    if (0 != texture.s.mip)
                    {
                        const float lodEnabled[4] = { float(texture.s.mip), 1.0f, 0.0f, 0.0f };
                        bgfx::setUniform(pBackend->u_imageLodEnabled, lodEnabled);
                        program = pBackend->m_imageProgram;
                    }
                }
                else
                {
                    state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
                }

                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clipRect;
                clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
                clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
                clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
                clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

                if (clipRect.x <  fb_width
                &&  clipRect.y <  fb_height
                &&  clipRect.z >= 0.0f
                &&  clipRect.w >= 0.0f)
                {
                    const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f) );
                    const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f) );
                    encoder->setScissor(xx, yy
                            , uint16_t(bx::min(clipRect.z, 65535.0f)-xx)
                            , uint16_t(bx::min(clipRect.w, 65535.0f)-yy)
                            );

                    encoder->setState(state);
                    encoder->setTexture(0, pBackend->s_tex, th);
                    encoder->setVertexBuffer(0, &tvb, 0, numVertices);
                    encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
                    encoder->submit(viewId, program);
                }
            }
        }

        bgfx::end(encoder);
    }
}

// ***********************************************************************

void OnRenderWindow(ImGuiViewport* pViewport, void*)
{
    if(ViewportData* pData = (ViewportData*)pViewport->RendererUserData; pData)
    {
        RenderView(pData->viewId, pViewport->DrawData, !(pViewport->Flags & ImGuiViewportFlags_NoRendererClear) ? 0x000000ff : 0);
    }
}

// ***********************************************************************

bool ProcessEvent(SDL_Event& event)
{
    if (ImGui_ImplSDL2_ProcessEvent(&event))
        return true;
    return false;
}

// ***********************************************************************

void UpdateCompilerExplorer() {

}

// ***********************************************************************

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

	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    ImGui_ImplSDL2_InitForD3D(pWindow);

    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    BackendData* pBackend = IM_NEW(BackendData)();
    io.BackendRendererUserData = (void*)pBackend;
    io.BackendRendererName = "imgui_bgfx";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

    // init vars, create shader programs, textures load fonts
    pBackend->m_SdlWindow = pWindow;
    pBackend->m_mainViewId = 255;

    bgfx::RendererType::Enum type = bgfx::getRendererType();
    pBackend->imguiVertex = bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui");
    pBackend->imguiFragment = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui");
    pBackend->imguiImageVertex = bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image");
    pBackend->imguiImageFragment = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image");

    pBackend->m_program = bgfx::createProgram(pBackend->imguiVertex, pBackend->imguiFragment, false);

    pBackend->u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
    pBackend->m_imageProgram = bgfx::createProgram(pBackend->imguiImageVertex, pBackend->imguiImageFragment, false);

    pBackend->m_layout
        .begin()
        .add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
        .end();

    pBackend->s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.MergeMode = false;

    const ImWchar* ranges = io.Fonts->GetGlyphRangesCyrillic();
    io.Fonts->AddFontFromMemoryTTF( (void*)s_robotoRegularTtf,     sizeof(s_robotoRegularTtf),     15.0,      &config, ranges);
    ImGui::StyleColorsDark();

    uint8_t* data;
    int32_t imgWidth;
    int32_t imgHeight;
    io.Fonts->GetTexDataAsRGBA32(&data, &imgWidth, &imgHeight);

    pBackend->m_texture = bgfx::createTexture2D(
            (uint16_t)imgWidth
        , (uint16_t)imgHeight
        , false
        , 1
        , bgfx::TextureFormat::BGRA8
        , 0
        , bgfx::copy(data, imgWidth*imgHeight*4)
        );

    io.Fonts->TexID = (void*)(intptr_t)pBackend->m_texture.idx;

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow = OnCreateWindow;
    platform_io.Renderer_DestroyWindow = OnDestroyWindow;
    platform_io.Renderer_SetWindowSize = OnSetWindowSize;
    platform_io.Renderer_RenderWindow = OnRenderWindow;

	while (gameRunning) {
		frameStartTime = SDL_GetPerformanceCounter();
		// Deal with events
		SDL_Event event;
		while (SDL_PollEvent(&event)){
			ProcessEvent(event);
			
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
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

		ImGui::ShowDemoWindow();
        UpdateCompilerExplorer();

        ImGui::Render();
        RenderView(pBackend->m_mainViewId, ImGui::GetDrawData(), 0);
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

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

    bgfx::destroy(pBackend->s_tex);
    bgfx::destroy(pBackend->m_texture);

    bgfx::destroy(pBackend->u_imageLodEnabled);
    bgfx::destroy(pBackend->m_imageProgram);
    bgfx::destroy(pBackend->m_program);

    ImGui_ImplSDL2_Shutdown();

	bgfx::shutdown();
}