// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <light_string.h>

#include "lua_common.h"

struct Image : public LuaObject {
    Image() {}
    Image(String path);
    virtual ~Image();

	// sokol todo
    // bgfx::TextureHandle handle { BGFX_INVALID_HANDLE };
    i32 width;
    i32 height;
};
