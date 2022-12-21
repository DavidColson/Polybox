// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <bgfx/bgfx.h>
#include <light_string.h>

#include "LuaCommon.h"

struct Image : public LuaObject {
    Image() {}
    Image(String path);
    virtual ~Image();

    bgfx::TextureHandle m_handle { BGFX_INVALID_HANDLE };
    int m_width;
    int m_height;
};
