// Copyright David Colson. All rights reserved.

#include "serialization.h"

#include "buffer.h"

#include <lua.h>
#include <lualib.h>
#include <string_builder.h>
#include <light_string.h>
#include <defer.h>
#include <log.h>

namespace Serialization {

void SerializeRecursive(lua_State* L, i32 depth, StringBuilder& builder) {
	// TODO:
	// Need to error if we encounter a cyclic table reference
	// need to deal with userdatas and other types

	if (lua_istable(L, -1)) {
		lua_pushnil(L);
		i32 arrayCounter = 0; // we'll set this to -1 if the sequence of indices breaks and we're in the hash part of the table
		while (lua_next(L, -2)) {
			lua_pushvalue(L, -2);

			// first decide if we're dealing with an array element or not
			if (arrayCounter >= 0 && lua_isnumber(L, -1)) {
				f32 key = lua_tonumber(L, -1);
				if (key != (i32)key || (i32)key != ++arrayCounter) {
					arrayCounter = -1;
				}
			} else {
				arrayCounter = -1;
			}

			// array element
			if (arrayCounter > 0) {
				lua_pop(L, 1); // pop key copy
				SerializeRecursive(L, 0, builder);
			}
			// dictionary element
			else {
				const char *key = lua_tostring(L, -1);
				if (lua_isnumber(L, -1)) {
					f32 key = lua_tonumber(L, -1);
					if (key == (i32)key) {
						builder.AppendFormat("[%i]=", (i32)key);
					}
					else {
						builder.AppendFormat("[%f]=", key);
					}
				}
				else {
					const char *key = lua_tostring(L, -1);
					// if this string has an equal character in it, we must escape it
					bool needsEscape = false;
					char* c = (char*)key;
					while (*c != 0) {
						if (*c == '=')
							needsEscape = true;
						c++;
					}
					if (needsEscape)
						builder.AppendFormat("[\"%s\"]=", key);
					else
						builder.AppendFormat("%s=", key);
				}

				lua_pop(L, 1); // pop key copy
				SerializeRecursive(L, 0, builder);
			}

			builder.AppendFormat(", ");
			lua_pop(L, 1);
		}
		// remove the final ", "
		builder.length -= 2;
	}
	if (lua_isuserdata(L, -1)) {
		if (lua_getmetatable(L, -1)) {
			lua_getfield(L, LUA_REGISTRYINDEX, "Buffer"); 
			if (lua_rawequal(L, -1, -2)) {
				lua_pop(L, 2);
				// value on stack is a buffer
				BufferLib::Buffer* pBuf = (BufferLib::Buffer*)lua_touserdata(L, -1);

				switch(pBuf->type) {
					case BufferLib::Type::Float64: builder.Append("buffer(\"f64\""); break;
					case BufferLib::Type::Float32: builder.Append("buffer(\"f32\""); break;
					case BufferLib::Type::Int64: builder.Append("buffer(\"i64\""); break;
					case BufferLib::Type::Int32: builder.Append("buffer(\"i32\""); break;
					case BufferLib::Type::Int16: builder.Append("buffer(\"i16\""); break;
					case BufferLib::Type::Uint8: builder.Append("buffer(\"u8\"");; break;
				}
				builder.AppendFormat(",%i,%i,\"", pBuf->width, pBuf->height);
				switch(pBuf->type) {
					case BufferLib::Type::Float64: {
						f64* pFloats = (f64*)pBuf->pData;
						for (int i = 0; i < pBuf->width*pBuf->height; i++) {
							if (i+1 < pBuf->width*pBuf->height)
								builder.AppendFormat("%f,", pFloats[i]);
							else
								builder.AppendFormat("%f", pFloats[i]);
						}
						break;
					}
					case BufferLib::Type::Float32: {
						f32* pFloats = (f32*)pBuf->pData;
						for (int i = 0; i < pBuf->width*pBuf->height; i++) {
							if (i+1 < pBuf->width*pBuf->height)
								builder.AppendFormat("%f,", pFloats[i]);
							else
								builder.AppendFormat("%f", pFloats[i]);
						}
						break;
					}
					case BufferLib::Type::Int64: {
						i64* pInts = (i64*)pBuf->pData;
						for (int i = 0; i < pBuf->width*pBuf->height; i++)
							builder.AppendFormat("%016x", pInts[i]);
						break;
					}
					case BufferLib::Type::Int32: {
						// 8 hex digits
						i32* pInts = (i32*)pBuf->pData;
						for (int i = 0; i < pBuf->width*pBuf->height; i++)
							builder.AppendFormat("%08x", pInts[i]);
						break;
					}
					case BufferLib::Type::Int16: {
						// 4 hex digits
						i16* pInts = (i16*)pBuf->pData;
						for (int i = 0; i < pBuf->width*pBuf->height; i++)
							builder.AppendFormat("%04x", pInts[i]);
						break;
					}
					case BufferLib::Type::Uint8: {
						// two hex digits == 1 byte
						u8* pInts = (u8*)pBuf->pData;
						for (int i = 0; i < pBuf->width*pBuf->height; i++)
							builder.AppendFormat("%02x", pInts[i]);
						break;
					}
				}
				builder.Append("\")");
			} else {
				luaL_error(L, "Unrecognized lua data, cannot be serialized");
			}
		} else { 
			luaL_error(L, "Unrecognized lua data, cannot be serialized");
		}
	}
	if (lua_isnumber(L, -1)) {
		f32 value = lua_tonumber(L, -1);
		if (value == (i32)value) {
			builder.AppendFormat("%i", (i32)value);
		}
		else {
			builder.AppendFormat("%f", value);
		}
	}
	else if(lua_isstring(L, -1)) {
		const char *value = lua_tostring(L, -2);
		builder.AppendFormat("\"%s\"", value);
	}
	else if (lua_isboolean(L, -1)) {
		bool value = lua_toboolean(L, -1);
		builder.AppendFormat("%s", value ? "true" : "false");
	}
	else if (lua_iscfunction(L, -1) || lua_isLfunction(L, -1)) {
		luaL_error(L, "Cannot serialize functions");
	}
}

i32 Serialize(lua_State* L) {
    StringBuilder builder;

    builder.Append("{");
	SerializeRecursive(L, 0, builder);
    builder.Append("}");

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
