#pragma once

#include "LuaCommon.h"

struct lua_State;
class GameChip;

namespace Bind
{
    int BindGameChip(lua_State* pLua, GameChip* pGameChip);
}