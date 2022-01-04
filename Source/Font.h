#pragma once

#include <string>
#include <vector>
#include <bgfx/bgfx.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "Core/Vec2.h"

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

struct Font
{
    void Load(std::string path, bool antialiasing = true, float weight = 0.0f);
    ~Font();

	bgfx::TextureHandle fontTexture;
	std::vector<Character> characters;
};

