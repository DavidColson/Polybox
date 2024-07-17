// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <light_string.h>
#include <sokol_gfx.h>

#include "lua_common.h"

struct Image : public LuaObject {
    Image() {}
    Image(String path);
    virtual ~Image();

	sg_image handle;
    i32 width;
    i32 height;
};
