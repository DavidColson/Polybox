// Copyright David Colson. All rights reserved.
#include "buffer.h"

#include <lua.h>
#include <lualib.h>
#include <memory.h>
#include <string.h>

namespace BufferLib {

i32 NewBuffer(lua_State* L) {
    usize len;
    const char* type = luaL_checklstring(L, 1, &len);
    i32 width = (i32)luaL_checkinteger(L, 2);
	i32 height = 1;
	if (lua_isnoneornil(L, 3) == 0) {
		height = (i32)luaL_checkinteger(L, 3);
	}

	Buffer* pBuffer = (Buffer*)lua_newuserdatadtor(L, sizeof(Buffer), [](void* pData) {
		Buffer* pBuffer = (Buffer*)pData;
		if (pBuffer) {
			g_Allocator.Free(pBuffer->pData);
		}
	});

	i32 typeSize = 0;
	if (strcmp(type, "f64") == 0) {
		pBuffer->type = Type::Float64;
		typeSize = sizeof(f64);
	}
	else if (strcmp(type, "f32") == 0) {
		pBuffer->type = Type::Float32;
		typeSize = sizeof(f32);
	}
	else if (strcmp(type, "i64") == 0) {
		pBuffer->type = Type::Int64;
		typeSize = sizeof(i64);
	}
	else if (strcmp(type, "i32") == 0) {
		pBuffer->type = Type::Int32;
		typeSize = sizeof(i32);
	}
	else if (strcmp(type, "i16") == 0) {
		pBuffer->type = Type::Int16;
		typeSize = sizeof(i16);
	}
	else if (strcmp(type, "u8") == 0) {
		pBuffer->type = Type::Uint8;
		typeSize = sizeof(u8);
	}
	else {
		luaL_error(L, "invalid type given to NewBuffer %s", type);
		return 0;
	}

	i32 bufSize = width * height * typeSize;
	pBuffer->pData = (char*)g_Allocator.Allocate(bufSize);
	memset(pBuffer->pData, 0, bufSize); 

	pBuffer->width = width;
	pBuffer->height = height;
 
	luaL_getmetatable(L, "Buffer");
	lua_setmetatable(L, -2);
	return 1;
}

void SetImpl(lua_State* L, Buffer* pBuffer, i32 index, i32 startParam) {
	// now grab as many integers as you can find and put them in the buffer from the starting index
	// while until none or nil
	i32 paramCounter = startParam;
	switch(pBuffer->type) {
		case Type::Float64: {
			f64* pData = (f64*)pBuffer->pData;
			pData += index;
			while (lua_isnoneornil(L, paramCounter) == 0) {
				*pData = (f64)luaL_checknumber(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Float32: {
			f32* pData = (f32*)pBuffer->pData;
			pData += index;
			while (lua_isnoneornil(L, paramCounter) == 0) {
				*pData = (f32)luaL_checknumber(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Int64: {
			i64* pData = (i64*)pBuffer->pData;
			pData += index;
			while (lua_isnoneornil(L, paramCounter) == 0) {
				*pData = (i64)luaL_checkinteger(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Int32: {
			i32* pData = (i32*)pBuffer->pData;
			pData += index;
			while (lua_isnoneornil(L, paramCounter) == 0) {
				*pData = (i32)luaL_checkinteger(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Int16: {
			i16* pData = (i16*)pBuffer->pData;
			pData += index;
			while (lua_isnoneornil(L, paramCounter) == 0) {
				*pData = (i16)luaL_checkinteger(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Uint8: {
			u8* pData = (u8*)pBuffer->pData;
			pData += index;
			while (lua_isnoneornil(L, paramCounter) == 0) {
				*pData = (u8)luaL_checkunsigned(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
	}
}

i32 Set(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
    i32 index = (i32)luaL_checkinteger(L, 2);
	SetImpl(L, pBuffer, index, 3);
	return 0;
}

i32 Set2D(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
	if (pBuffer->height == 1) {
		luaL_error(L, "Set2D is only valid on 2-dimensional buffers");
		return 0;
	}

    i32 x = (i32)luaL_checkinteger(L, 2);
    i32 y = (i32)luaL_checkinteger(L, 3);
	SetImpl(L, pBuffer, pBuffer->width * y + x, 4);
	return 0;
}

i32 GetImpl(lua_State* L, Buffer* pBuffer, i32 index, i32 count) {
	switch(pBuffer->type) {
		case Type::Float64: {
			f64* pData = (f64*)pBuffer->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushnumber(L, pData[i]);
			}
			break;
		}
		case Type::Float32: {
			f32* pData = (f32*)pBuffer->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushnumber(L, pData[i]);
			}
			break;
		}
		case Type::Int64: {
			i64* pData = (i64*)pBuffer->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushinteger(L, pData[i]);
			}
			break;
		}
		case Type::Int32: {
			i32* pData = (i32*)pBuffer->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushinteger(L, pData[i]);
			}
			break;
		}
		case Type::Int16: {
			i16* pData = (i16*)pBuffer->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushinteger(L, pData[i]);
			}
			break;
		}
		case Type::Uint8: {
			u8* pData = (u8*)pBuffer->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushunsigned(L, pData[i]);
			}
			break;
		}
	}

	return count;
}

i32 Get(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
    i32 index = (i32)luaL_checkinteger(L, 2);
	i32 count = (i32)luaL_checkinteger(L, 3);
	return GetImpl(L, pBuffer, index, count);
}

i32 Get2D(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
	if (pBuffer->height == 1) {
		luaL_error(L, "Get2D is only valid on 2-dimensional buffers");
		return 0;
	}
    i32 x = (i32)luaL_checkinteger(L, 2);
    i32 y = (i32)luaL_checkinteger(L, 3);
	i32 count = (i32)luaL_checkinteger(L, 3);
	return GetImpl(L, pBuffer, pBuffer->width * y + x, count);
}

void BindBuffer(lua_State* L) {

	// register global functions
    const luaL_Reg globalFuncs[] = {
        { "NewBuffer", NewBuffer },
        { NULL, NULL }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, NULL, globalFuncs);
    lua_pop(L, 1);

	// new metatable
	luaL_newmetatable(L, "Buffer");

	// register metamethods
    const luaL_Reg bufferMethods[] = {
        { "Set", Set },
        { "Set2D", Set2D },
        { "Get", Get },
        { "Get2D", Get2D },
        { NULL, NULL }
    };

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

	luaL_register(L, NULL, bufferMethods);
    lua_pop(L, 1);
}

}
