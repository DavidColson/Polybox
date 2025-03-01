
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
	scd.SampleDesc.Quality = 0;
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

void SokolFlush() {
	_sg.d3d11.ctx->Flush();
}

// ***********************************************************************

void SokolPresent() {
	// TODO: this is where you'd handle screen resizing, it's called once per frame
	pSwapChain->Present(1, 0);
}

// ***********************************************************************

uint32_t _sg_d3d11_dxgi_format_to_sdl_pixel_format(DXGI_FORMAT dxgi_format) {
    switch(dxgi_format) {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return SDL_PIXELFORMAT_ARGB8888;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return SDL_PIXELFORMAT_ABGR8888;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return SDL_PIXELFORMAT_RGB888;
        default:
            return SDL_PIXELFORMAT_UNKNOWN;
    }
}

// ***********************************************************************

void ReadbackImagePixels(sg_image img_id, void* pixels) {
	_sg_image_t* img = _sg_lookup_image(&_sg.pools, img_id.id);

    // create staging texture
    ID3D11Texture2D* staging_tex = NULL;
    D3D11_TEXTURE2D_DESC staging_desc = {
        .Width = (UINT)img->cmn.width,
        .Height = (UINT)img->cmn.height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = img->d3d11.format,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0,
        },
        .Usage = D3D11_USAGE_STAGING,
        .BindFlags = 0,
        .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
        .MiscFlags = 0
    };
    _sg_d3d11_CreateTexture2D(_sg.d3d11.dev, &staging_desc, NULL, &staging_tex);

    // copy pixels to staging texture
	_sg.d3d11.ctx->CopySubresourceRegion(
        (ID3D11Resource*)staging_tex,
        0, 0, 0, 0,
        (ID3D11Resource*)img->d3d11.tex2d,
        0, NULL);

    // map the staging texture's data to CPU-accessible memory
    D3D11_MAPPED_SUBRESOURCE msr = {.pData = NULL};
    _sg_d3d11_Map(_sg.d3d11.ctx, (ID3D11Resource*)staging_tex, 0, D3D11_MAP_READ, 0, &msr);

    // copy the data into the desired buffer, converting pixels to the desired format at the same time
    int res = SDL_ConvertPixels(
        img->cmn.width, img->cmn.height,
        _sg_d3d11_dxgi_format_to_sdl_pixel_format(staging_desc.Format),
        msr.pData, msr.RowPitch,
        SDL_PIXELFORMAT_RGBA32,
        pixels, img->cmn.width * 4);
    _SOKOL_UNUSED(res);

    // unmap the texture
    _sg_d3d11_Unmap(_sg.d3d11.ctx, (ID3D11Resource*)staging_tex, 0);

    if(staging_tex) _sg_d3d11_Release(staging_tex);
}

// ***********************************************************************

void ReadbackPixels(int x, int y, int w, int h, void *pixels) {
    // get current render target
    ID3D11RenderTargetView* render_target_view = NULL;
	_sg.d3d11.ctx->OMGetRenderTargets(1, &render_target_view, NULL);

    // fallback to window render target
    if(!render_target_view)
        render_target_view = (ID3D11RenderTargetView*)_sg.d3d11.cur_pass.render_view;

	if (render_target_view == nullptr)
		return;

    // get the back buffer texture
    ID3D11Texture2D *back_buffer = NULL;
	render_target_view->GetResource((ID3D11Resource**)&back_buffer);

    // create a staging texture to copy the screen's data to
    D3D11_TEXTURE2D_DESC staging_desc;
	back_buffer->GetDesc(&staging_desc);
    staging_desc.Width = w;
    staging_desc.Height = h;
    staging_desc.BindFlags = 0;
    staging_desc.MiscFlags = 0;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    ID3D11Texture2D *staging_tex = NULL;
    _sg_d3d11_CreateTexture2D(_sg.d3d11.dev, &staging_desc, NULL, &staging_tex);

    // copy the desired portion of the back buffer to the staging texture
    D3D11_BOX src_box = {
        .left = (UINT)x,
        .top = (UINT)y,
        .front = 0,
        .right = (UINT)(x + w),
        .bottom = (UINT)(y + h),
        .back = 1,
    };
	_sg.d3d11.ctx->CopySubresourceRegion(
		(ID3D11Resource*)staging_tex,
        0, 0, 0, 0,
        (ID3D11Resource*)back_buffer,
        0, &src_box);

    // map the staging texture's data to CPU-accessible memory
    D3D11_MAPPED_SUBRESOURCE msr = {.pData = NULL};
    _sg_d3d11_Map(_sg.d3d11.ctx, (ID3D11Resource*)staging_tex, 0, D3D11_MAP_READ, 0, &msr);

    // copy the data into the desired buffer, converting pixels to the desired format at the same time
    int res = SDL_ConvertPixels(
        w, h,
        _sg_d3d11_dxgi_format_to_sdl_pixel_format(staging_desc.Format),
        msr.pData, msr.RowPitch,
        SDL_PIXELFORMAT_RGBA32,
        pixels, w * 4);
    _SOKOL_UNUSED(res);

    // unmap the texture
    _sg_d3d11_Unmap(_sg.d3d11.ctx, (ID3D11Resource*)staging_tex, 0);

    if(back_buffer) _sg_d3d11_Release(back_buffer);
    if(staging_tex) _sg_d3d11_Release(staging_tex);
}



// FOR future reference/porting work, here is a metal and opengl implementation from here: 
// https://github.com/edubart/sokol_gp/blob/beedb873724c53eb56fe3539882218e7a92aac80/sokol_gfx_ext.h

// OpenGL
// Note, a better way here might be pixel buffer objects. Something to consider when porting
#if 0

static void _sg_gl_query_image_pixels(_sg_image_t* img, void* pixels) {
    SOKOL_ASSERT(img->gl.target == GL_TEXTURE_2D);
    SOKOL_ASSERT(0 != img->gl.tex[img->cmn.active_slot]);
#if defined(SOKOL_GLCORE33)
    _sg_gl_cache_store_texture_binding(0);
    _sg_gl_cache_bind_texture(0, img->gl.target, img->gl.tex[img->cmn.active_slot]);
    glGetTexImage(img->gl.target, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    _SG_GL_CHECK_ERROR();
    _sg_gl_cache_restore_texture_binding(0);
#else
    static GLuint newFbo = 0;
    GLuint oldFbo = 0;
    if(newFbo == 0) {
        glGenFramebuffers(1, &newFbo);
    }
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&oldFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, newFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, img->gl.tex[img->cmn.active_slot], 0);
    glReadPixels(0, 0, img->cmn.width, img->cmn.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
    //glDeleteFramebuffers(1, &newFbo);
    _SG_GL_CHECK_ERROR();
#endif
}

static void _sg_gl_query_pixels(int x, int y, int w, int h, void *pixels) {
    SOKOL_ASSERT(pixels);
    GLuint gl_fb;
    GLint dims[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&gl_fb);
    _SG_GL_CHECK_ERROR();
    glGetIntegerv(GL_VIEWPORT, dims);
    int cur_height = dims[3];
    _SG_GL_CHECK_ERROR();
#if defined(SOKOL_GLES2) // use NV extension instead
    glReadBufferNV(gl_fb == 0 ? GL_BACK : GL_COLOR_ATTACHMENT0);
#else
    glReadBuffer(gl_fb == 0 ? GL_BACK : GL_COLOR_ATTACHMENT0);
#endif
    _SG_GL_CHECK_ERROR();
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    _SG_GL_CHECK_ERROR();
}

#endif


// Metal
#if 0

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

static uint32_t _sg_metal_texture_format_to_sdl_pixel_format(MTLPixelFormat texture_format) {
    switch(texture_format) {
        case MTLPixelFormatBGRA8Unorm:
            return SDL_PIXELFORMAT_ARGB8888;
        case MTLPixelFormatRGBA8Unorm:
            return SDL_PIXELFORMAT_ABGR8888;
        default:
            return SDL_PIXELFORMAT_UNKNOWN;
    }
}

static void _sg_metal_commit_command_buffer() {
    SOKOL_ASSERT(!_sg.mtl.in_pass);
    if(_sg.mtl.cmd_buffer) {
        #if defined(_SG_TARGET_MACOS)
        [_sg.mtl.uniform_buffers[_sg.mtl.cur_frame_rotate_index] didModifyRange:NSMakeRange(0, _sg.mtl.cur_ub_offset)];
        #endif
        [_sg.mtl.cmd_buffer commit];
        [_sg.mtl.cmd_buffer waitUntilCompleted];
        _sg.mtl.cmd_buffer = [_sg.mtl.cmd_queue commandBufferWithUnretainedReferences];
    }
}

static void _sg_metal_encode_texture_pixels(int x, int y, int w, int h, id<MTLTexture> mtl_src_texture, void* pixels) {
    SOKOL_ASSERT(!_sg.mtl.in_pass);
    _sg_metal_commit_command_buffer();
    MTLTextureDescriptor* mtl_dst_texture_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtl_src_texture.pixelFormat width:w height:h mipmapped:NO];
    mtl_dst_texture_desc.storageMode = MTLStorageModeManaged;
    mtl_dst_texture_desc.resourceOptions = MTLResourceStorageModeManaged;
    mtl_dst_texture_desc.usage = MTLTextureUsageShaderRead + MTLTextureUsageShaderWrite;
    id<MTLTexture> mtl_dst_texture = [mtl_src_texture.device newTextureWithDescriptor:mtl_dst_texture_desc];
    id<MTLCommandBuffer> cmd_buffer = [_sg.mtl.cmd_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
    [blit_encoder copyFromTexture:mtl_src_texture
        sourceSlice:0
        sourceLevel:0
        sourceOrigin:MTLOriginMake(x,y,0)
        sourceSize:MTLSizeMake(w,h,1)
        toTexture:mtl_dst_texture
        destinationSlice:0
        destinationLevel:0
        destinationOrigin:MTLOriginMake(0,0,0)
    ];
    [blit_encoder synchronizeTexture:mtl_dst_texture slice:0 level:0];
    [blit_encoder endEncoding];
    [cmd_buffer commit];
    [cmd_buffer waitUntilCompleted];

    MTLRegion mtl_region = MTLRegionMake2D(0, 0, w, h);
    void* temp_pixels = (void*)SOKOL_MALLOC(w * 4 * h);
    SOKOL_ASSERT(temp_pixels);
    [mtl_dst_texture getBytes:temp_pixels bytesPerRow:w * 4 fromRegion:mtl_region mipmapLevel:0];
    int res = SDL_ConvertPixels(w, h, _sg_metal_texture_format_to_sdl_pixel_format(mtl_dst_texture_desc.pixelFormat), temp_pixels, w * 4, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
    SOKOL_FREE(temp_pixels);
    SOKOL_ASSERT(res == 0);
    _SOKOL_UNUSED(res);
}

static void _sg_metal_query_image_pixels(_sg_image_t* img, void* pixels) {
    id<MTLTexture> mtl_src_texture = _sg.mtl.idpool.pool[img->mtl.tex[0]];
    _sg_metal_encode_texture_pixels(0, 0, mtl_src_texture.width, mtl_src_texture.height, true, mtl_src_texture, pixels);
}

static void _sg_metal_query_pixels(int x, int y, int w, int h, void *pixels) {
    id<CAMetalDrawable> mtl_drawable = (__bridge id<CAMetalDrawable>)_sg.mtl.drawable_cb();
    _sg_metal_encode_texture_pixels(x, y, w, h, mtl_drawable.texture, pixels);
}

#endif
