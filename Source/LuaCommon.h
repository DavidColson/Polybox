#pragma once

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

namespace Lua
{
    // Note to dave: we don't need the registry table that love2d has since we don't have any functions that give back instances of things
    // Keep in mind for now that if a function returns a udata to some object that may already exist, you MAY want to prevent it making 
    // multiple copies of the udata all pointing to the same c++ type instance. Given that each udata in our implementation is just a pointer,
    // Maybe that's fine to just let it have multiple udatas

    struct Object
    {
        virtual void Free() = 0;
    };

    void RegisterNewType(lua_State* pLua, const char* typeName, const luaL_Reg *funcs);
}