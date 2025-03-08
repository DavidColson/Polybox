// Copyright David Colson. All rights reserved.

struct lua_State;

i32 Serialize(lua_State* L);
int Deserialize(lua_State* L);
void BindSerialization(lua_State* L);
