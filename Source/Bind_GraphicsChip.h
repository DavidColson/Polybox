// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "LuaCommon.h"

struct lua_State;
class GraphicsChip;

namespace Bind
{
    int BindGraphicsChip(lua_State* pLua, GraphicsChip* pGraphics);
}