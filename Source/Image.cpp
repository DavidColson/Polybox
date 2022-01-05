#include "Image.h"

#include <bimg/decode.h>
#include <bimg/bimg.h>
#include <bx/bx.h>
#include <bx/allocator.h>
#include <bx/error.h>
#include <SDL_rwops.h>

// ***********************************************************************

void ImageFreeCallback(void* ptr, void* userData)
{
    bimg::ImageContainer* pContainer = (bimg::ImageContainer*)userData;
    bimg::imageFree(pContainer);
}

// ***********************************************************************

Image::Image(std::string path)
{
    // Load and cache texture files and upload to bgfx
    SDL_RWops* pFileRead = SDL_RWFromFile(path.c_str(), "rb");

    uint64_t size = SDL_RWsize(pFileRead);
    void* pData = new char[size];
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);
    
    static bx::DefaultAllocator allocator;
    bx::Error error;
    bimg::ImageContainer* pContainer = bimg::imageParse(&allocator, pData, (uint32_t)size, bimg::TextureFormat::Count, &error);

    const bgfx::Memory* pMem = bgfx::makeRef(pContainer->m_data, pContainer->m_size, ImageFreeCallback, pContainer);
    m_handle = bgfx::createTexture2D(pContainer->m_width, pContainer->m_height, 1 < pContainer->m_numMips, pContainer->m_numLayers, bgfx::TextureFormat::Enum(pContainer->m_format), BGFX_TEXTURE_NONE|BGFX_SAMPLER_POINT, pMem);
    m_width = pContainer->m_width;
    m_height = pContainer->m_height;

    bgfx::setName(m_handle, path.c_str());
    delete pData;
}