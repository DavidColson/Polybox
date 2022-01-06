#include "LuaCommon.h"

// ***********************************************************************

static int GarbageCollect(lua_State* pLua)
{
    Lua::Object *pObject = *(Lua::Object**)lua_touserdata(pLua, 1);
    if (pObject)
    {
        pObject->Free();
    }
    return 0;
}

// ***********************************************************************

static int EqualityCheck(lua_State* pLua)
{
    Lua::Object *pObject1 = *(Lua::Object**)lua_touserdata(pLua, 1);
    Lua::Object *pObject2 = *(Lua::Object**)lua_touserdata(pLua, 1);

    if (pObject1 && pObject2)
    {
        bool equal = pObject1 == pObject2;
        lua_pushboolean(pLua, (int)equal);
        return 1;
    }
    else
    {
        // TODO Throw error as passed in values are not userdatas
    }
    lua_pushboolean(pLua, (int)false);
    return 1;
}

// ***********************************************************************

static int GetType(lua_State *L)
{
	lua_pushvalue(L, lua_upvalueindex(1));
	return 1;
}

// ***********************************************************************

void Lua::RegisterNewType(lua_State* pLua, const char* typeName, const luaL_Reg *funcs)
{
    luaL_newmetatable(pLua, typeName);

    // sets table.__index = table 
    lua_pushvalue(pLua, -1);
    lua_setfield(pLua, -2, "__index");

    lua_pushcfunction(pLua, GarbageCollect);
    lua_setfield(pLua, -2, "__gc");

    lua_pushcfunction(pLua, EqualityCheck);
    lua_setfield(pLua, -2, "__eq");

    lua_pushstring(pLua, typeName);
	lua_pushcclosure(pLua, GetType, 1);
	lua_setfield(pLua, -2, "GetType");

    // Push all of the type member functions into the metatable
    luaL_setfuncs(pLua, funcs, 0);

    lua_pop(pLua, 1);
}