// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

struct lua_State;
class GameChip;

namespace Bind {
int BindGameChip(lua_State* pLua, GameChip* pGameChip);
}
