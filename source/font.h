// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <resizable_array.h>
#include <vec2.h>

#include "image.h"
#include "lua_common.h"

struct String;

struct Character {
    Vec2i size { Vec2i(0, 0) };
    Vec2i bearing { Vec2i(0, 0) };
    Vec2f UV0 { Vec2f(0.f, 0.f) };
    Vec2f UV1 { Vec2f(1.f, 1.f) };
    i32 advance;
};

struct Font : public LuaObject {
	Font() {}
	virtual ~Font();

	void Initialize(String path, bool antialiasing = true, f32 weight = 0.0f);

	Arena* pArena;
    Image fontTexture;
    ResizableArray<Character> characters;
};
