// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "types.h"

struct lua_State;
struct luaL_Reg;

// Some helpers and extensions to the lua C library

struct LuaObject {
    LuaObject();
    LuaObject(const LuaObject& other);
    virtual ~LuaObject() {}

    void Retain();
    void Release();

    i32 refCount { 0 };
};

void luax_printstack(lua_State* pLua);
bool luax_toboolean(lua_State* L, i32 idx);
bool luax_checkboolean(lua_State* pLua, i32 idx);
void luax_registertype(lua_State* pLua, const char* typeName, const luaL_Reg* funcs);
