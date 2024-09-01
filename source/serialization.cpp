// Copyright David Colson. All rights reserved.

#include "serialization.h"

#include "buffer.h"

#include <lua.h>
#include <lualib.h>
#include <string_builder.h>
#include <resizable_array.inl>
#include <light_string.h>
#include <defer.h>
#include <log.h>
#include <scanning.h>
#include <cstring>

namespace Serialization {

// ***********************************************************************

void SerializeTextRecursive(lua_State* L, StringBuilder& builder) {
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
				SerializeTextRecursive(L, builder);
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
				SerializeTextRecursive(L, builder);
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
					case BufferLib::Type::Float32: builder.Append("buffer(f32"); break;
					case BufferLib::Type::Int32: builder.Append("buffer(i32"); break;
					case BufferLib::Type::Int16: builder.Append("buffer(i16"); break;
					case BufferLib::Type::Uint8: builder.Append("buffer(u8");; break;
				}
				builder.AppendFormat(",%i,%i,\"", pBuf->width, pBuf->height);
				switch(pBuf->type) {
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

void CborEncode(ResizableArray<byte>& output, u8 majorType, byte* pData, size dataSize) {
	u8 additionalInfo;
	u8 followingBytes;

	// check if upper 32 bits are set, therefore needs 8 bytes of space
	if (dataSize & ~(0x100000000ull - 1)) {
		additionalInfo = 27;
		followingBytes = 8;
	// upper 16 bits this time (4 bytes)
	} else if (dataSize & ~(0x10000ull - 1)) {
		additionalInfo = 26;
		followingBytes = 4;
	// upper 8 bits (2 bytes)
	} else if (dataSize & ~(0x100ull - 1)) {
		additionalInfo = 25;
		followingBytes = 2;
	// upper 4 bits set (1 byte)
	} else if (dataSize >= 24) {
		additionalInfo = 24;
		followingBytes = 1;
	// remainder 0 - 23
	} else {
		additionalInfo = (u8)dataSize;
		followingBytes = 0;
	}

	size requiredSize = (size)dataSize + followingBytes + 1; // extra 1 for header
	if (!(majorType == 2 || majorType == 3)) {
		requiredSize -= (size)dataSize;
	}

	// Allocate more space in output if needed
	output.Reserve(output.GrowCapacity(output.capacity + requiredSize));

	output.pData[output.count] = (u8)majorType << 5 | additionalInfo;
	memcpy(&output.pData[output.count+1], (u8*)&dataSize, followingBytes);
	if (pData != nullptr) {
		// copy pData to output
	}

	output.count += requiredSize;
}

// ***********************************************************************

void SerializeCborRecursive(lua_State* L, ResizableArray<byte>& output) {

	if (lua_istable(L, -1)) {
		luaL_error(L, "tables not supported yet");
	}
	if (lua_isuserdata(L, -1)) {
		luaL_error(L, "buffers not supported yet");
	}
	if (lua_isnumber(L, -1)) {
		f32 value = lua_tonumber(L, -1);
		// Dave, test ints next
		if (value == (i32)value) {
			if (value > 0) {
				CborEncode(output, 0, nullptr, value);
			} else {
				CborEncode(output, 1, nullptr, (-value)-1);
			}
		}
		else {
			// float (major type 7)
		}
	}
	else if(lua_isstring(L, -1)) {
		luaL_error(L, "strings not supported yet");
	}
	else if (lua_isboolean(L, -1)) {
		luaL_error(L, "bools not supported yet");
	}
	else if (lua_iscfunction(L, -1) || lua_isLfunction(L, -1)) {
		luaL_error(L, "Cannot serialize functions");
	}
}

// ***********************************************************************

i32 Serialize(lua_State* L) {

	i32 mode = (i32)luaL_checknumber(L, 2);

	// Text serialization
	if (mode == 1) {
		StringBuilder builder;
		SerializeTextRecursive(L, builder);

		String result = builder.CreateString();
		defer(FreeString(result));

		// return string of serialized code
		lua_pushstring(L, result.pData);

		// Binary (cbor) serialization
	} else if (mode == 2) {
		ResizableArray<byte> result;
		SerializeCborRecursive(L, result);

		// Note, this is not a valid lua string? Will it cause issues?
		// Will it cause corruptions? Probably it will strcopy up to a 0 then stop
		// You could push it back as a buffer?

		lua_pushstring(L, result.pData);
		result.Free();

		// Do cbor I guess?
		// Ultimately this will still be a recursive walk of the lua values

		// for each value we encode a cbor data item

		// For number:
		// if positive integer of zero: major type 0
		// if less than 24, put in next 3 bytes
		// Depending on size, but in next 1, 2, 4, 8 bytes and set previous 5 bytes to 24, 25, 26, 27 respectively
		// if negative integer: major type 1
		// calculate the value -1-val and then do as above
		// if float: major type 7
		// 26 in next 3 bytes
		// the 32 bit float value

		// for string:
		// Major type 3, no escaping
		// Place string length using integer encoding above
		// The string, as is follows

		// For table
		// Major type 5
		// Then serialize integer for number of elements in table
		// then serialize pairs, key value using appropiate item encoding

		// For boolean
		// Major item 7,
		// next 3 bytes are 20 for false, and 21 for true

		// for buffer
		// Major item 2,
		// encode integer as above, with length of buffer struct size + pData size
		// encode the buffer struct first, then the pData 
	}
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
	// we assume the buffer word has been parsed already
	// so match (
	if (!Scan::Match(scan, '('))
		luaL_error(L, "Expected '(' to start buffer");

	// create the buffer
	BufferLib::Buffer* pBuffer = (BufferLib::Buffer*)lua_newuserdatadtor(L, sizeof(BufferLib::Buffer), [](void* pData) {
		BufferLib::Buffer* pBuffer = (BufferLib::Buffer*)pData;
		if (pBuffer) {
			g_Allocator.Free(pBuffer->pData);
		}
	});

	// parse an identifier to get the type
	byte* pStart = scan.pCurrent;
	while (Scan::Peek(scan) != ',') 
		Scan::Advance(scan);

	String identifier;
	identifier.pData = pStart;
	identifier.length = scan.pCurrent - pStart;

	i32 typeSize = 0;
	if (identifier == "f32") {
		pBuffer->type = BufferLib::Type::Float32;
		typeSize = sizeof(f32);
	}
	else if (identifier == "i32") {
		pBuffer->type = BufferLib::Type::Int32;
		typeSize = sizeof(i32);
	}
	else if (identifier == "i16") {
		pBuffer->type = BufferLib::Type::Int16;
		typeSize = sizeof(i16);
	}
	else if (identifier == "u8") {
		pBuffer->type = BufferLib::Type::Uint8;
		typeSize = sizeof(u8);
	}
	else {
		luaL_error(L, "Unexpected buffer value type");
	}

	Scan::Advance(scan); // ,
	if (Scan::IsDigit(Scan::Advance(scan)))
		pBuffer->width = (i32)ParseNumber(scan);
	else 
		luaL_error(L, "Expected number for width");

	if (!Scan::Match(scan, ','))
		luaL_error(L, "Expected ',' between arguments");

	if (Scan::IsDigit(Scan::Advance(scan)))
		pBuffer->height = (i32)ParseNumber(scan);
	else
		luaL_error(L, "Expected number for height");

	// Actually alloc the buffer
	i32 bufSize = pBuffer->width * pBuffer->height * typeSize;
	pBuffer->pData = (char*)g_Allocator.Allocate(bufSize);
	memset(pBuffer->pData, 0, bufSize); 

	if (!Scan::Match(scan, ','))
		luaL_error(L, "Expected ',' between arguments");

	// then quote to start value string
	if (!Scan::Match(scan, '\"'))
		luaL_error(L, "Expected '\"' to start data block");

	// switch on type
	switch(pBuffer->type) {
		case BufferLib::Type::Float32: {
			f32* pData = (f32*)pBuffer->pData;
			while (Scan::Peek(scan) != ')') {
				scan.pCurrent++; // parse number expects that the first digit has been parsed
				*pData = ParseNumber(scan);
				if (Scan::Peek(scan) != '\"' && Scan::Peek(scan) != ',')
					luaL_error(L, "Expected ',' between values");
					else
					Scan::Advance(scan);
				pData++;
			}
			break;
		}
		case BufferLib::Type::Int32: {
			char number[9];
			number[8] = 0;
			i32* pData = (i32*)pBuffer->pData;
			while (Scan::Peek(scan) != '\"') {
				number[0] = *scan.pCurrent++;
				number[1] = *scan.pCurrent++;
				number[2] = *scan.pCurrent++;
				number[3] = *scan.pCurrent++;
				number[4] = *scan.pCurrent++;
				number[5] = *scan.pCurrent++;
				number[6] = *scan.pCurrent++;
				number[7] = *scan.pCurrent++;
				*pData = (i32)strtol(number, nullptr, 16);
				pData++;
			}
			Scan::Advance(scan);
			break;
		}
		case BufferLib::Type::Int16: {
			char number[5];
			number[4] = 0;
			i16* pData = (i16*)pBuffer->pData;
			while (Scan::Peek(scan) != '\"') {
				number[0] = *scan.pCurrent++;
				number[1] = *scan.pCurrent++;
				number[2] = *scan.pCurrent++;
				number[3] = *scan.pCurrent++;
				*pData = (i16)strtol(number, nullptr, 16);
				pData++;
			}
			Scan::Advance(scan);
			break;
		}
		case BufferLib::Type::Uint8: {
			char number[3];
			number[2] = 0;
			u8* pData = (u8*)pBuffer->pData;
			while (Scan::Peek(scan) != '\"') {
				number[0] = *scan.pCurrent++;
				number[1] = *scan.pCurrent++;
			*pData = (u8)strtol(number, nullptr, 16);
				pData++;
			}
			Scan::Advance(scan);
			break;
		}
	}
	if (!Scan::Match(scan, ')')) 
		luaL_error(L, "Expected ')' to end buffer");

	luaL_getmetatable(L, "Buffer");
	lua_setmetatable(L, -2);
}

// ***********************************************************************

void ParseValue(lua_State* L, Scan::ScanningState& scan) {
	while (!Scan::IsAtEnd(scan)) {
		Scan::AdvanceOverWhitespace(scan);
		byte c = Scan::Advance(scan);

		if (Scan::IsAlpha(c)) {
			byte* pStart = scan.pCurrent - 1;
			while (Scan::IsAlphaNumeric(Scan::Peek(scan)))
				Scan::Advance(scan);

			String identifier = CopyCStringRange(pStart, scan.pCurrent, &g_Allocator);
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
