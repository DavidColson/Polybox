#include "sokol_impl.h"

#include "d3d11.h"
#include "dxgi.h"

#include <SDL_syswm.h>

#define SOKOL_GFX_IMPL
#define SOKOL_D3D11
#include <sokol_gfx.h>

namespace {
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pDeviceContext = nullptr;
IDXGISwapChain* pSwapChain = nullptr;
ID3D11Texture2D* pRenderTarget = nullptr;
ID3D11RenderTargetView* pRenderTargetView = nullptr;
ID3D11Texture2D* pDepthStencil = nullptr;
ID3D11DepthStencilView* pDepthStencilView = nullptr;
int winWidth = 0;
int winHeight = 0;
}

// ***********************************************************************

bool GraphicsBackendInit(SDL_Window* pWindow, int width, int height) {
	winWidth = width;
	winHeight = height;
	
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(pWindow, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;

	// Create DirectX device
	DXGI_SWAP_CHAIN_DESC scd = {0};
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hwnd;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
	scd.Windowed = true;

	if (D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&scd,
		&pSwapChain,
		&pDevice,
		NULL,
		&pDeviceContext) != S_OK) {
		return false;
	}

	// Create render target and depth stencil
	{
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pRenderTarget);
		pDevice->CreateRenderTargetView((ID3D11Resource*)pRenderTarget, nullptr, &pRenderTargetView);

		D3D11_TEXTURE2D_DESC dsd = {
			.Width = (UINT)width,
			.Height = (UINT)height,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
			.SampleDesc = {.Count = (UINT)1, .Quality = (UINT)0 },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_DEPTH_STENCIL,
		};
		pDevice->CreateTexture2D(&dsd, nullptr, &pDepthStencil);
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {
			.Format = dsd.Format, .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS
		};
		pDevice->CreateDepthStencilView((ID3D11Resource*)pDepthStencil, &dsvd, &pDepthStencilView);
	}
	return true;
}

// ***********************************************************************

sg_environment SokolGetEnvironment() {
	return sg_environment{
        .defaults = {
            .color_format = SG_PIXELFORMAT_BGRA8,
            .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .sample_count = 1,
        },
        .d3d11 = {
            .device = pDevice,
            .device_context = pDeviceContext,
        }
    };
}

// ***********************************************************************

sg_swapchain SokolGetSwapchain() {
	return sg_swapchain{
        .width = winWidth,
        .height = winHeight,
        .sample_count = 1,
        .color_format = SG_PIXELFORMAT_BGRA8,
        .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
        .d3d11 = {
            .render_view = pRenderTargetView,
            .resolve_view = 0,
            .depth_stencil_view = pDepthStencilView,
        }
    };
}

// ***********************************************************************

void SokolPresent() {
	// TODO: this is where you'd handle screen resizing, it's called once per frame
	pSwapChain->Present(1, 0);
}
