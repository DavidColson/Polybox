#pragma once

#include "LuaCommon.h"

struct lua_State;

namespace Bind
{
    int BindScene(lua_State* pLua);
}