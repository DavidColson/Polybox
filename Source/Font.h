// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <light_string.h>
#include <vector>
#include <bgfx/bgfx.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "Core/Vec2.h"
#include "Image.h"
#include "LuaCommon.h"

namespace FreeType
{
	FT_Library* Get();
}

struct Character
{
	Vec2i size{ Vec2i(0, 0) };
	Vec2i bearing{ Vec2i(0, 0) };
	Vec2f UV0{ Vec2f(0.f, 0.f) };
	Vec2f UV1{ Vec2f(1.f, 1.f) };
	int advance;
};

struct Font : public LuaObject
{
    Font() {}
    Font(String path, bool antialiasing = true, float weight = 0.0f);

	Image fontTexture;
	std::vector<Character> characters;
};

