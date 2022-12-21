// Copyright 2020-2022 David Colson. All rights reserved.

#include "Image.h"

#include <light_string.h>

#include <SDL_rwops.h>
#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <bx/allocator.h>
#include <bx/bx.h>
#include <bx/error.h>

// ***********************************************************************

void ImageFreeCallback(void* ptr, void* userData) {
    bimg::ImageContainer* pContainer = (bimg::ImageContainer*)userData;
    bimg::imageFree(pContainer);
}

// ***********************************************************************

Image::Image(String path) {
    // Load and cache texture files and upload to bgfx
    SDL_RWops* pFileRead = SDL_RWFromFile(path.m_pData, "rb");

    uint64_t size = SDL_RWsize(pFileRead);
    void* pData = g_Allocator.Allocate(size * sizeof(char));
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);

    static bx::DefaultAllocator allocator;
    bx::Error error;
    bimg::ImageContainer* pContainer = bimg::imageParse(&allocator, pData, (uint32_t)size, bimg::TextureFormat::Count, &error);

    const bgfx::Memory* pMem = bgfx::makeRef(pContainer->m_data, pContainer->m_size, ImageFreeCallback, pContainer);
    m_handle = bgfx::createTexture2D(pContainer->m_width, pContainer->m_height, 1 < pContainer->m_numMips, pContainer->m_numLayers, bgfx::TextureFormat::Enum(pContainer->m_format), BGFX_TEXTURE_NONE | BGFX_SAMPLER_POINT, pMem);
    m_width = pContainer->m_width;
    m_height = pContainer->m_height;

    bgfx::setName(m_handle, path.m_pData);
    g_Allocator.Free(pData);
}

// ***********************************************************************

Image::~Image() {
    if (m_refCount <= 0)
        bgfx::destroy(m_handle);
}
