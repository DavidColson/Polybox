// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "LuaCommon.h"

struct lua_State;

namespace Bind
{
    int BindScene(lua_State* pLua);
}