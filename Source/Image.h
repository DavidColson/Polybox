#pragma once

#include <string>
#include <bgfx/bgfx.h>

struct Image
{
    Image() {}
    Image(std::string path);
    ~Image() {}

	bgfx::TextureHandle m_handle{ BGFX_INVALID_HANDLE };
    int m_width;
    int m_height;
};

