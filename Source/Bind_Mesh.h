#pragma once

#include "LuaCommon.h"

struct lua_State;

namespace Bind
{
    int BindMeshTypes(lua_State* pLua);
}