#pragma once

#include <string>
#include <bgfx/bgfx.h>

#include "LuaCommon.h"

struct Image : public LuaObject
{
    Image() {}
    Image(std::string path);
    virtual ~Image();

	bgfx::TextureHandle m_handle{ BGFX_INVALID_HANDLE };
    int m_width;
    int m_height;
};

