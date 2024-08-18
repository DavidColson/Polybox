// Copyright David Colson. All rights reserved.

#include "serialization.h"

#include "buffer.h"

#include <lua.h>
#include <lualib.h>
#include <string_builder.h>
#include <light_string.h>
#include <defer.h>
#include <log.h>
#include <scanning.h>

namespace Serialization {

// ***********************************************************************

void SerializeRecursive(lua_State* L, i32 depth, StringBuilder& builder) {
	// TODO:
	// Need to error if we encounter a cyclic table reference
	// need to deal with userdatas and other types

	if (lua_istable(L, -1)) {
		builder.Append("{");
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
		builder.Append("}");
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

// ***********************************************************************

i32 Serialize(lua_State* L) {
    StringBuilder builder;

	SerializeRecursive(L, 0, builder);

	String result = builder.CreateString();
	defer(FreeString(result));

	// return string of serialized code
	lua_pushstring(L, result.pData);
	return 1;
}

// ***********************************************************************

void ParseValue(lua_State* L, Scan::ScanningState& scan);

// ***********************************************************************

void ParseTable(lua_State* L, Scan::ScanningState& scan) {
	lua_newtable(L);
	i32 arrayIndex = 1;

	// While loop over each table element
	while (!Scan::IsAtEnd(scan)) {
		Scan::AdvanceOverWhitespace(scan);
        byte c = Scan::Advance(scan);

		// Identifier
		if (Scan::IsAlpha(c)) {
			byte* start = scan.pCurrent - 1;
			while (Scan::IsAlphaNumeric(Scan::Peek(scan)))
				Scan::Advance(scan);

			String identifier = CopyCStringRange(start, scan.pCurrent, &g_Allocator);
			defer(FreeString(identifier));

			if (identifier == "true" || identifier == "false" || identifier == "buffer") {
				ParseValue(L, scan);

				lua_rawseti(L, -2, arrayIndex);
				arrayIndex++;
				continue;
			}

			if (!Scan::Match(scan, '=')) {
				luaL_error(L, "Expected '=' after table key", size(scan.pCurrent - scan.pTextStart));
			}

			ParseValue(L, scan);
			lua_setfield(L, -2, identifier.pData);
			continue;
		}

		// Is value
		if (Scan::IsDigit(c) || c == '\'' || c == '"') {
			scan.pCurrent--;
			ParseValue(L, scan);
			lua_rawseti(L, -2, arrayIndex);
			arrayIndex++;
			continue;
		}

		// Is string or number identifier
		if (c == '[') {
			// put key on stack
			ParseValue(L, scan);

			if (!Scan::Match(scan, ']')) {
				luaL_error(L, "Expected ']' after table key", size(scan.pCurrent - scan.pTextStart));
			}
			if (!Scan::Match(scan, '=')) {
				luaL_error(L, "Expected '=' after table key", size(scan.pCurrent - scan.pTextStart));
			}

			ParseValue(L, scan);

			lua_settable(L, -3);
			continue;
		}

		if (c != ',') {
			if (c == '}') {
				return; // table should already be top of stack
			}
			luaL_error(L, "Expected '}' to end table", size(scan.pCurrent - scan.pTextStart));
		}
	}
}

// ***********************************************************************

void ParseBuffer(lua_State* L, Scan::ScanningState& scan) {

}

// ***********************************************************************

void ParseValue(lua_State* L, Scan::ScanningState& scan) {
	while (!Scan::IsAtEnd(scan)) {
		Scan::AdvanceOverWhitespace(scan);
        byte c = Scan::Advance(scan);

		if (Scan::IsAlpha(c)) {
			byte* start = scan.pCurrent - 1;
			while (Scan::IsAlphaNumeric(Scan::Peek(scan)))
				Scan::Advance(scan);

			String identifier = CopyCStringRange(start, scan.pCurrent, &g_Allocator);
			defer(FreeString(identifier));

			if (identifier == "true")
				lua_pushboolean(L, 1);
			else if (identifier == "false")
				lua_pushboolean(L, 0);
			else if (identifier == "buffer") {
				ParseBuffer(L, scan);
			}
			else {
				luaL_error(L, "Unexpected identifier in data %s", identifier.pData);
			}
			return;
		}

		// number
		if (Scan::IsDigit(c)) {
			f64 num = ParseNumber(scan);
			lua_pushnumber(L, num);
			return;
		}

		//string
		if (c == '\'' || c == '"') {
			String string = ParseString(&g_Allocator, scan, '\"');
			lua_pushstring(L, string.pData);
			FreeString(string);
			return;
		}

		// table
		if (c == '{') {
			ParseTable(L, scan);
			return;
		}

		luaL_error(L, "Unexpected character in data to serialize. At location %i", size(scan.pCurrent - scan.pTextStart));
	}
}

// ***********************************************************************

int Deserialize(lua_State* L) {
    usize len;
    const char* str = luaL_checklstring(L, 1, &len);

	Scan::ScanningState scan;
	scan.pTextStart = str;
	scan.pTextEnd = str + len;
	scan.pCurrent = (byte*)scan.pTextStart;
	scan.line = 1;

	ParseValue(L, scan);
	
	return 1;
}

// ***********************************************************************

void BindSerialization(lua_State* L) {
	// register global functions
    const luaL_Reg globalFuncs[] = {
        { "serialize", Serialize },
        { "deserialize", Deserialize },
        { NULL, NULL }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, NULL, globalFuncs);
    lua_pop(L, 1);
}

}
