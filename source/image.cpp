// Copyright 2020-2022 David Colson. All rights reserved.

#include "Image.h"

#include <light_string.h>

#include <SDL_rwops.h>

// ***********************************************************************

void ImageFreeCallback(void* ptr, void* userData) {
    // bimg::ImageContainer* pContainer = (bimg::ImageContainer*)userData;
    // bimg::imageFree(pContainer);
}

// ***********************************************************************

Image::Image(String path) {



	// sokol todo, might need to get stb_image in to do this now

    // Load and cache texture files and upload to bgfx
    // SDL_RWops* pFileRead = SDL_RWFromFile(path.pData, "rb");
    //
    // u64 size = SDL_RWsize(pFileRead);
    // void* pData = g_Allocator.Allocate(size * sizeof(char));
    // SDL_RWread(pFileRead, pData, size, 1);
    // SDL_RWclose(pFileRead);
    //
    // static bx::DefaultAllocator allocator;
    // bx::Error error;
    // bimg::ImageContainer* pContainer = bimg::imageParse(&allocator, pData, (u32)size, bimg::TextureFormat::Count, &error);
    //
    // const bgfx::Memory* pMem = bgfx::makeRef(pContainer->m_data, pContainer->m_size, ImageFreeCallback, pContainer);
    // handle = bgfx::createTexture2D(pContainer->m_width, pContainer->m_height, 1 < pContainer->m_numMips, pContainer->m_numLayers, bgfx::TextureFormat::Enum(pContainer->m_format), BGFX_TEXTURE_NONE | BGFX_SAMPLER_POINT, pMem);
    // width = pContainer->m_width;
    // height = pContainer->m_height;
    //
    // bgfx::setName(handle, path.pData);
    // g_Allocator.Free(pData);
}

// ***********************************************************************

Image::~Image() {
    // if (refCount <= 0)
    //     bgfx::destroy(handle);
}
