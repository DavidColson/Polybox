// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <bgfx/bgfx.h>
#include <ft2build.h>
#include <light_string.h>
#include <resizable_array.h>
#include <vec2.h>
#include FT_FREETYPE_H

#include "image.h"
#include "lua_common.h"

namespace FreeType {
FT_Library* Get();
}

struct Character {
    Vec2i size { Vec2i(0, 0) };
    Vec2i bearing { Vec2i(0, 0) };
    Vec2f UV0 { Vec2f(0.f, 0.f) };
    Vec2f UV1 { Vec2f(1.f, 1.f) };
    int advance;
};

struct Font : public LuaObject {
    Font() {}
    Font(String path, bool antialiasing = true, float weight = 0.0f);

    Image fontTexture;
    ResizableArray<Character> characters;
};