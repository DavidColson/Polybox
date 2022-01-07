#pragma once

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

// Some helpers and extensions to the lua C library

struct LuaObject
{
	LuaObject();
	LuaObject(const LuaObject& other);
	virtual ~LuaObject() {}

    void Retain();
    void Release();

	int m_refCount{ 0 };
};

bool luax_toboolean(lua_State *L, int idx);
bool luax_checkboolean(lua_State* pLua, int idx);
void luax_registertype(lua_State* pLua, const char* typeName, const luaL_Reg *funcs);
