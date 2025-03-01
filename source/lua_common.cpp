// Copyright 2020-2022 David Colson. All rights reserved.

#undef DrawText
#undef DrawTextEx

// ***********************************************************************

LuaObject::LuaObject() {
    refCount = 1;
}

// ***********************************************************************

LuaObject::LuaObject(const LuaObject& other) {
    refCount = 1;
}

// ***********************************************************************

void LuaObject::Retain() {
    refCount++;
}

// ***********************************************************************

void LuaObject::Release() {
    refCount--;
    if (refCount == 0) {
        delete this;
    }
}

// ***********************************************************************

static i32 __equality(lua_State* pLua) {
    LuaObject* pObject1 = *(LuaObject**)lua_touserdata(pLua, 1);
    LuaObject* pObject2 = *(LuaObject**)lua_touserdata(pLua, 1);

    if (pObject1 && pObject2) {
        bool equal = pObject1 == pObject2;
        lua_pushboolean(pLua, (int)equal);
        return 1;
    } else {
        // TODO Throw error as passed in values are not userdatas
    }
    lua_pushboolean(pLua, (int)false);
    return 1;
}

// ***********************************************************************

static i32 __gettype(lua_State* L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    return 1;
}

// ***********************************************************************

void luax_printstack(lua_State* pLua) {
    Log::Debug("STACK");
    for (int i = 1; i <= lua_gettop(pLua); i++) {
        Log::Debug("%i - %s", i, luaL_typename(pLua, i));
    }
}

// ***********************************************************************

bool luax_toboolean(lua_State* pLua, i32 idx) {
    return (lua_toboolean(pLua, idx) != 0);
}

// ***********************************************************************

bool luax_checkboolean(lua_State* pLua, i32 idx) {
    luaL_checktype(pLua, idx, LUA_TBOOLEAN);
    return luax_toboolean(pLua, idx);
}

// ***********************************************************************

void luax_registertype(lua_State* pLua, const char* typeName, const luaL_Reg* funcs) {
    luaL_newmetatable(pLua, typeName);

    // sets table.__index = table
    lua_pushvalue(pLua, -1);
    lua_setfield(pLua, -2, "__index");

    lua_pushcfunction(pLua, __equality, nullptr);
    lua_setfield(pLua, -2, "__eq");

    lua_pushstring(pLua, typeName);
    lua_pushcclosure(pLua, __gettype, nullptr, 1);
    lua_setfield(pLua, -2, "GetType");

    if (funcs) {
        // Push all of the type member functions into the metatable
		luaL_register(pLua, NULL, funcs);
    }

    lua_pop(pLua, 1);
}
