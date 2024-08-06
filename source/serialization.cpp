// Copyright David Colson. All rights reserved.

#include "serialization.h"

#include <lua.h>
#include <lualib.h>
#include <string_builder.h>
#include <light_string.h>
#include <defer.h>

namespace Serialization {

i32 Serialize(lua_State* L) {
	// what did I recieve? If it's a table proceed with table iteration

    StringBuilder builder;

    builder.Append("{\n");

	if (lua_istable(L, 1)) {
		lua_pushnil(L);
		// stack now contains: -1 => nil; -2 => table
		while (lua_next(L, -2))
		{
			// stack now contains: -1 => value; -2 => key; -3 => table
			// copy the key so that lua_tostring does not modify the original
			lua_pushvalue(L, -2);
			// stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
			const char *key = lua_tostring(L, -1);
			if(lua_isnumber(L, -2)) {
				f32 value = lua_tonumber(L, -2);
				if (value == (i32)value) {
					builder.AppendFormat("	\"%s\" = %i,\n", key, (i32)value);
				}
				else {
					builder.AppendFormat("	\"%s\" = %f,\n", key, value);
				}
			}
			else if(lua_isboolean(L, -2)) {
				bool value = lua_toboolean(L, -2);
				builder.AppendFormat("	\"%s\" = %s,\n", key, value ? "true" : "false");
			}
			else if (lua_isstring(L, -2)) {
				const char *value = lua_tostring(L, -2);
				builder.AppendFormat("	\"%s\" = \"%s\",\n", key, value);
			}
			// pop value + copy of key, leaving original key
			lua_pop(L, 2);
			// stack now contains: -1 => key; -2 => table
		}	
	}

    builder.Append("}\n");

	String result = builder.CreateString();
	defer(FreeString(result));

	// return string of serialized code
	lua_pushstring(L, result.pData);
	return 1;
}

void BindSerialization(lua_State* L) {
	// register global functions
    const luaL_Reg globalFuncs[] = {
        { "serialize", Serialize },
        { NULL, NULL }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, NULL, globalFuncs);
    lua_pop(L, 1);
}

}
