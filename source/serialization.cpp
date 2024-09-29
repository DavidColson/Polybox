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
#include <lz4.h>
#include <base64.h>
#include <SDL_rwops.h>
#include <stdlib.h>

namespace Serialization {

// ***********************************************************************

void SerializeTextRecursive(lua_State* L, StringBuilder& builder, bool isMetadata = false) {
	// TODO:
	// Need to error if we encounter a cyclic table reference

	if (lua_istable(L, -1)) {
		if (isMetadata) {
			builder.Append("--[[poly,");
		}
		else {
			builder.Append("{");
		}
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
		if (isMetadata) {
			builder.Append("]]");
		}
		else {
			builder.Append("}");
		}
	}
	if (lua_isuserdata(L, -1)) {
		if (lua_getmetatable(L, -1)) {
			lua_getfield(L, LUA_REGISTRYINDEX, "Buffer"); 
			if (lua_rawequal(L, -1, -2)) {
				lua_pop(L, 2);
				// value on stack is a buffer
				BufferLib::Buffer* pBuf = (BufferLib::Buffer*)lua_touserdata(L, -1);

				switch(pBuf->type) {
					case BufferLib::Type::Float32: builder.Append("buffer(\"f32\""); break;
					case BufferLib::Type::Int32: builder.Append("buffer(\"i32\""); break;
					case BufferLib::Type::Int16: builder.Append("buffer(\"i16\""); break;
					case BufferLib::Type::Uint8: builder.Append("buffer(\"u8\"");; break;
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
		const char *value = lua_tostring(L, -1);
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

size MemcpyBE(u8 *dst, u8 *src, size len)
{
	for (size_t i = 0; i < len; i++) {
		dst[len - i - 1] = src[i];
	}

	return len;
}

// ***********************************************************************

size MemcpyLE(u8 *dst, u8 *src, size len)
{
	for (size_t i = 0; i < len; i++) {
		dst[i] = src[i];
	}

	return len;
}

// ***********************************************************************

void CborEncode(ResizableArray<u8>& output, u8 majorType, u8* pData, size dataSize, bool indefinite = false) {
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

	if (indefinite) {
		additionalInfo = 31;
		followingBytes = 0;
	}

	size requiredSize = (size)dataSize + followingBytes + 1; // extra 1 for header
	if (!(majorType == 2 || majorType == 3)) {
		requiredSize -= (size)dataSize;
	}

	// Allocate more space in output if needed
	output.Reserve(output.GrowCapacity(output.count + requiredSize));

	output.pData[output.count] = (u8)majorType << 5 | additionalInfo;
	MemcpyBE(&output.pData[output.count+1], (u8*)&dataSize, followingBytes);
	if (pData != nullptr) {
		MemcpyLE(&output.pData[output.count+1+followingBytes], pData, dataSize);
	}

	output.count += requiredSize;
}

// ***********************************************************************

void SerializeCborRecursive(lua_State* L, ResizableArray<u8>& output) {
	if (lua_istable(L, -1)) {
		// iterate over table checking keys, want to see if this is an array or map
		lua_pushnil(L);
		bool isArray = true;
		i32 arrayLen = 1;
		while (lua_next(L, -2)) {
			lua_pushvalue(L, -2);

			if (lua_isnumber(L, -1)) {
				i32 key = (i32)lua_tonumber(L, -1);
				if (key == arrayLen) {
					arrayLen++;
					lua_pop(L, 2);
					continue;
				}
			}
			isArray = false;
			lua_pop(L, 3);
			break;
		}
		arrayLen--;

		if (isArray) {
			CborEncode(output, 4, nullptr, arrayLen);
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				SerializeCborRecursive(L, output);
				lua_pop(L, 1);
			}
		}
		else {
			CborEncode(output, 5, nullptr, 0, true);
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				lua_pushvalue(L, -2);
				// key
				SerializeCborRecursive(L, output);
				lua_pop(L, 1);
				// value
				SerializeCborRecursive(L, output);
				lua_pop(L, 1);
			}
			CborEncode(output, 7, nullptr, 0, true);
		}
	}
	if (lua_isuserdata(L, -1)) {
		if (lua_getmetatable(L, -1)) {
			lua_getfield(L, LUA_REGISTRYINDEX, "Buffer"); 
			if (lua_rawequal(L, -1, -2)) {
			lua_pop(L, 2);
				// value on stack is a buffer
				BufferLib::Buffer* pBuf = (BufferLib::Buffer*)lua_touserdata(L, -1);

				size udataSize = pBuf->width * pBuf->height;
				switch(pBuf->type) {
					case BufferLib::Type::Float32: udataSize *= sizeof(f32); break;
					case BufferLib::Type::Int32: udataSize *= sizeof(i32); break;
					case BufferLib::Type::Int16: udataSize *= sizeof(i16); break;
					case BufferLib::Type::Uint8: udataSize *= sizeof(u8); break;
				}
				size requiredSpace = udataSize + sizeof(i32) + sizeof(i32) + 1;

				CborEncode(output, 2, nullptr, requiredSpace);
				output.count -= requiredSpace;
				MemcpyLE(&output.pData[output.count], (u8*)&pBuf->width, 4);
				MemcpyLE(&output.pData[output.count + 4], (u8*)&pBuf->height, 4);
				MemcpyLE(&output.pData[output.count + 8], (u8*)&pBuf->type, 1);
				MemcpyLE(&output.pData[output.count + 9], (u8*)pBuf->pData, udataSize);
				output.count += requiredSpace;
			} else {
				luaL_error(L, "Unrecognized lua data, cannot be serialized");
			}
		} else { 
			luaL_error(L, "Unrecognized lua data, cannot be serialized");
		}
	}
	if (lua_isnumber(L, -1)) {
		f64 value = lua_tonumber(L, -1);
		if (value == (i32)value) {
			if (value > 0) {
				CborEncode(output, 0, nullptr, value);
			} else {
				CborEncode(output, 1, nullptr, (-value)-1);
			}
		}
		else {
			f32 single = (f32)value;	
			u8 majorType = 7;
			if ((f64)single == value) {
				// can be safely stored in single
				size requiredSize = 5;
				output.Reserve(output.GrowCapacity(output.count + requiredSize));
				output.pData[output.count] = majorType << 5 | 26;
				MemcpyBE(&output.pData[output.count+1], (u8*)&single, 4);
				output.count += requiredSize;
			} else {
				// Needs double precision
				size requiredSize = 9;
				output.Reserve(output.GrowCapacity(output.count + requiredSize));
				output.pData[output.count] = majorType << 5 | 27;
				MemcpyBE(&output.pData[output.count+1], (u8*)&value, 8);
				output.count += requiredSize;
			}
		}
	}
	else if(lua_isstring(L, -1)) {
		const char *str = lua_tostring(L, -1);
		CborEncode(output, 3, (u8*)str, strlen(str));
	}
	else if (lua_isboolean(L, -1)) {
		bool value = lua_toboolean(L, -1);
		CborEncode(output, 7, nullptr, (u8)(value + 20));
	}
	else if (lua_iscfunction(L, -1) || lua_isLfunction(L, -1)) {
		luaL_error(L, "Cannot serialize functions");
	}
}

// ***********************************************************************

i32 Serialize(lua_State* L) {
	bool isStore = false;
	if (lua_isstring(L, 1))
		isStore = true;

	i32 mode = (i32)luaL_checknumber(L, isStore ? 3 : 2);

	String metaData;

	if (lua_gettop(L) >= 3) {
		StringBuilder builder(g_pArenaFrame);
		SerializeTextRecursive(L, builder, true);
		metaData = builder.CreateString(g_pArenaFrame);
	}

	// put the value on the top of the stack for the following functions
	lua_pushvalue(L, isStore ? 2:1);

	// Text serialization
	if (mode == 0x0) {
		StringBuilder builder(g_pArenaFrame);

		if (metaData.length != 0)
			builder.Append(metaData);

		SerializeTextRecursive(L, builder);

		String result = builder.CreateString(g_pArenaFrame);

		// return string of serialized code
		lua_pushstring(L, result.pData);
		return 1;
	}

	ResizableArray<u8> output(g_pArenaFrame);

	// if binary, then serialize to cbor into output
	if (mode & 0x1 || mode & 0x2 || mode & 0x4) {
		output.Reserve(output.GrowCapacity(1));
		output.PushBack(0xBD); // identifier

		SerializeCborRecursive(L, output);
	}

	// if compressed, then compress output and resize
	if (mode & 0x2) {
		size srcSize = output.count - 1;
		size compressBound = LZ4_compressBound(srcSize);
		u8* pData = output.pData + 1; // skip the binary identifier bit

		u8* stagingBuffer = New(g_pArenaFrame, u8, compressBound);

		i32 compressedSize = LZ4_compress_default((char*)pData, (char*)stagingBuffer, srcSize, compressBound);
		if (compressedSize == 0) {
			luaL_error(L, "Compression failed");
		}	

		output.Reserve(output.GrowCapacity(9 + compressedSize));

		output.count = 0;
		output.PushBack(0xBC);
		MemcpyLE((u8*)output.pData+1, (u8*)&compressedSize, 4); 			// write compressed size
		MemcpyLE((u8*)output.pData+5, (u8*)&srcSize, 4); 				// write original size
		MemcpyLE((u8*)output.pData+9, stagingBuffer, compressedSize); 	// write compressed data
		output.count += 8 + compressedSize;
	}

	// base 64 encoded binary
	if (mode & 0x4) {
		String encoded = EncodeBase64(g_pArenaFrame, output.count, output.pData);

		output.count = 4 + encoded.length;
		output.Reserve(output.GrowCapacity(output.count));
		const char* head = "b64:";
		memcpy(output.pData, head, 4);
		memcpy(output.pData + 4, encoded.pData, encoded.length);
	}

	if (metaData.length != 0) { 
		output.Reserve(output.GrowCapacity(output.count + metaData.length));
		memmove(output.pData + metaData.length, output.pData, output.count);
		memcpy(output.pData, metaData.pData, metaData.length);
		output.count += metaData.length;
	}

	lua_pushlstring(L, (byte*)output.pData, output.count);
	return 1;
}

// ***********************************************************************

void ParseTextValue(lua_State* L, Scan::ScanningState& scan);
void ParseBuffer(lua_State* L, Scan::ScanningState& scan);

// ***********************************************************************

void ParseTextTable(lua_State* L, Scan::ScanningState& scan, bool isMetadata = false) {
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

			String identifier = CopyCStringRange(start, scan.pCurrent, g_pArenaFrame);

			if (identifier == "true") {
				lua_pushboolean(L, 1);
				lua_rawseti(L, -2, arrayIndex);
				arrayIndex++;
				continue;
			}
			else if (identifier == "false") {
				lua_pushboolean(L, 0);
				lua_rawseti(L, -2, arrayIndex);
				arrayIndex++;
				continue;
			}
			else if (identifier == "buffer") {
				if (Scan::Peek(scan) == '(') {
					ParseBuffer(L, scan);
					lua_rawseti(L, -2, arrayIndex);
					arrayIndex++;
					continue;
				}
			}

			if (!Scan::Match(scan, '=')) {
				luaL_error(L, "Expected '=' after table key", size(scan.pCurrent - scan.pTextStart));
			}

			ParseTextValue(L, scan);
			lua_setfield(L, -2, identifier.pData);
			continue;
		}

		// Is value
		if (Scan::IsDigit(c) || c == '\'' || c == '"') {
			scan.pCurrent--;
			ParseTextValue(L, scan);
			lua_rawseti(L, -2, arrayIndex);
			arrayIndex++;
			continue;
		}

		// Is string or number identifier
		if (c == '[') {
			// put key on stack
			ParseTextValue(L, scan);

			if (!Scan::Match(scan, ']')) {
				luaL_error(L, "Expected ']' after table key", size(scan.pCurrent - scan.pTextStart));
			}
			if (!Scan::Match(scan, '=')) {
				luaL_error(L, "Expected '=' after table key", size(scan.pCurrent - scan.pTextStart));
			}

			ParseTextValue(L, scan);

			lua_settable(L, -3);
			continue;
		}

		if (c != ',') {
			if (isMetadata && strcmp(scan.pCurrent-1, "]]")) {
				return; // end of metadata table
			}
			else if (c == '}') {
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

	i32 bufferWidth = 1;
	i32 bufferHeight = 1;
	BufferLib::Type bufferType = BufferLib::Type::Int32;

	// parse an identifier to get the type
	byte* pStart = scan.pCurrent;
	while (Scan::Peek(scan) != ',') 
		Scan::Advance(scan);

	String identifier;
	identifier.pData = pStart;
	identifier.length = scan.pCurrent - pStart;

	i32 typeSize = 0;
	if (identifier == "\"f32\"") {
		bufferType = BufferLib::Type::Float32;
		typeSize = sizeof(f32);
	}
	else if (identifier == "\"i32\"") {
		bufferType = BufferLib::Type::Int32;
		typeSize = sizeof(i32);
	}
	else if (identifier == "\"i16\"") {
		bufferType = BufferLib::Type::Int16;
		typeSize = sizeof(i16);
	}
	else if (identifier == "\"u8\"") {
		bufferType = BufferLib::Type::Uint8;
		typeSize = sizeof(u8);
	}
	else {
		luaL_error(L, "Unexpected buffer value type");
	}

	Scan::Advance(scan); // ,
	if (Scan::IsDigit(Scan::Advance(scan)))
		bufferWidth = (i32)ParseNumber(scan);
	else 
		luaL_error(L, "Expected number for width");

	if (!Scan::Match(scan, ','))
		luaL_error(L, "Expected ',' between arguments");

	if (Scan::IsDigit(Scan::Advance(scan)))
		bufferHeight = (i32)ParseNumber(scan);
	else
		luaL_error(L, "Expected number for height");

	// Actually alloc the buffer
	BufferLib::Buffer* pBuffer = BufferLib::AllocBuffer(L, bufferType, bufferWidth, bufferHeight);

	if (!Scan::Match(scan, ','))
		luaL_error(L, "Expected ',' between arguments");

	// then quote to start value string
	if (!Scan::Match(scan, '\"'))
		luaL_error(L, "Expected '\"' to start data block");

	// parse data string
	String dataString;
	dataString.pData = scan.pCurrent;
	dataString.length = 0;
	while (Scan::Advance(scan) != '\"')
		dataString.length++;
	BufferLib::ParseBufferDataString(L, dataString, pBuffer);

	if (!Scan::Match(scan, ')')) 
		luaL_error(L, "Expected ')' to end buffer");
}

// ***********************************************************************

void ParseTextValue(lua_State* L, Scan::ScanningState& scan) {
	while (!Scan::IsAtEnd(scan)) {
		Scan::AdvanceOverWhitespace(scan);
		byte c = Scan::Advance(scan);

		if (Scan::IsAlpha(c)) {
			byte* pStart = scan.pCurrent - 1;
			while (Scan::IsAlphaNumeric(Scan::Peek(scan)))
				Scan::Advance(scan);

			String identifier = CopyCStringRange(pStart, scan.pCurrent, g_pArenaFrame);

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
			String string = ParseString(g_pArenaFrame, scan, '\"');
			lua_pushstring(L, string.pData);
			return;
		}

		// table
		if (c == '{') {
			ParseTextTable(L, scan);
			return;
		}
	

		luaL_error(L, "Unexpected character in data to serialize. At location %i", size(scan.pCurrent - scan.pTextStart));
	}
}

// ***********************************************************************

struct CborParserState {
	u8* pData;
	u8* pEnd;
	u8* pCurrent;
	i32 len;
};

// ***********************************************************************

bool ParseCborValue(lua_State* L, CborParserState& state, i32 maxItems = -1) {
	i32 items = 0;
	while ((maxItems == -1 || items < maxItems) && state.pCurrent < state.pEnd) {
		u8 val = *state.pCurrent;
		items++;

		// get major type and additional information
		u8 majorType = val >> 5; // top 3 bits
		u8 additionalInfo = val & 0x1fu; // bottom 5 bits

		u8 followingBytes = 1u << (additionalInfo - 24);

		if (additionalInfo < 24) {
			followingBytes = 0;
		}
		else if (additionalInfo == 31 && majorType == 7) { 
			// stop code of invalid info
			state.pCurrent++;
			return true;
		}

		state.pCurrent++;

		switch(majorType) {
			case 0:{ // positive int
				if (followingBytes == 0) {
					lua_pushnumber(L, additionalInfo);
				} else {
					i32 result = 0;
					MemcpyBE((u8*)&result, (u8*)state.pCurrent, followingBytes);
					lua_pushnumber(L, result);
					state.pCurrent += followingBytes;
				}
				break;
			}
			case 1: // negative int
				if (followingBytes == 0) {
					lua_pushnumber(L, -1-(i32)additionalInfo);
				} else {
					i32 result = 0;
					MemcpyBE((u8*)&result, (u8*)state.pCurrent, followingBytes);
					lua_pushnumber(L, -1-result);
					state.pCurrent += followingBytes;
				}
				break;
			case 2: { // buffer
				i32 dataLen = 0;
				if (followingBytes == 0) {
					dataLen = additionalInfo;
				} else {
					MemcpyBE((u8*)&dataLen, (u8*)state.pCurrent, followingBytes);
					state.pCurrent += followingBytes;
				}

				i32 bufferWidth = 0;
				i32 bufferHeight = 0;
				BufferLib::Type bufferType = BufferLib::Type::Int32;
				MemcpyLE((u8*)&bufferWidth, (u8*)state.pCurrent, 4);
				state.pCurrent += 4;
				MemcpyLE((u8*)&bufferHeight, (u8*)state.pCurrent, 4);
				state.pCurrent += 4;
				MemcpyLE((u8*)&bufferType, (u8*)state.pCurrent, 1);
				state.pCurrent += 1;

				BufferLib::Buffer* pBuffer = BufferLib::AllocBuffer(L, bufferType, bufferWidth, bufferHeight);
				i32 bufSize = pBuffer->width * pBuffer->height;
				switch(pBuffer->type) {
					case BufferLib::Type::Float32: bufSize *= sizeof(f32); break;
					case BufferLib::Type::Int32: bufSize *= sizeof(i32); break;
					case BufferLib::Type::Int16: bufSize *= sizeof(i16); break;
					case BufferLib::Type::Uint8: bufSize *= sizeof(u8); break;
				}
				MemcpyLE((u8*)pBuffer->pData, (u8*)state.pCurrent, bufSize);
				state.pCurrent += bufSize;
				break;
			}
			case 3: { // string
				i32 strlen = 0;
				if (followingBytes == 0) {
					strlen = additionalInfo;
				} else {
					MemcpyBE((u8*)&strlen, (u8*)state.pCurrent, followingBytes);
					state.pCurrent += followingBytes;
				}
				String str = CopyCStringRange((char*)state.pCurrent, (char*)state.pCurrent + strlen, g_pArenaFrame);

				lua_pushstring(L, str.pData);
				state.pCurrent += strlen;
				break;
			}
			case 4: { // array
				lua_newtable(L);

				i32 arrayLen = 0;
				if (followingBytes == 0) {
					arrayLen = additionalInfo;
				} else {
					MemcpyBE((u8*)&arrayLen, (u8*)state.pCurrent, followingBytes);
					state.pCurrent += followingBytes;
				}
				
				for (int i = 1; i < arrayLen + 1; i++) {
					ParseCborValue(L, state, 1);
					lua_rawseti(L, -2, i);
				}
				break;
			}
			case 5: { // table
				lua_newtable(L);

				// we always encode tables with a break at the end, so we don't care about this
				if (additionalInfo != 31) {
					luaL_error(L, "Map encoded without a stop code, this is unsupported here");
				}

				while (true) {
					if (ParseCborValue(L, state, 1)) {
						break;
					}
					ParseCborValue(L, state, 1);
					lua_settable(L, -3);
				}
				break;
			}
			case 6: // unsupported
				luaL_error(L, "Unsupported cbor type encountered when deserializing (6)");
				break;
			case 7: { // floats and bools
				if (additionalInfo == 20) {
					lua_pushboolean(L, false);
				}
				else if (additionalInfo == 21) {
					lua_pushboolean(L, true);
				}
				else if (additionalInfo == 26) {
					f32 result = 0.f;
					MemcpyBE((u8*)&result, (u8*)state.pCurrent, 4);
					state.pCurrent += 4;
					lua_pushnumber(L, result);
				}
				else if (additionalInfo == 27) {
					f64 result = 0.0;
					MemcpyBE((u8*)&result, (u8*)state.pCurrent, 8);
					state.pCurrent += 8;
					lua_pushnumber(L, result);
				}
				else {
					luaL_error(L, "Unexpected data, potentially something is corrupt");
				}
				break;
			}
			default:
				break;
		}
	}
	return false;
}

// ***********************************************************************

int Deserialize(lua_State* L) {
	usize len;
	const char* str = luaL_checklstring(L, -1, &len);

	// find metadata if possible
	char* start = (char*)str;
	if (strncmp(str, "--[[poly,", 9) == 0) {
		while (true) {
			if (strncmp(start, "]]", 2) == 0) {
				start += 2;
				break;
			}
			start++;
		}
	}

	i32 metaDataLen = start - str;
	i32 dataLen = len - metaDataLen;

	String metaData;
	metaData.pData = (char*)str + 9; // 9 is the length of '--[[poly,'
	metaData.length = metaDataLen - 9;

	String decoded;

	if (strncmp(start, "b64:", 4) == 0) {
		// base64
		String base64;
		base64.pData = (byte*)start + 4;
		base64.length = dataLen - 4;

		decoded = DecodeBase64(g_pArenaFrame, base64);
	} else {
		decoded.pData = (byte*)start;
		decoded.length = dataLen;
	}

	if ((ubyte)decoded.pData[0] == 0xBD) {
		// binary file
		CborParserState state;
		state.len = decoded.length - 1;
		state.pData = (u8*)decoded.pData+1;
		state.pEnd = state.pData + state.len;
		state.pCurrent = state.pData;

		ParseCborValue(L, state);
	}
	else if ((ubyte)decoded.pData[0] == 0xBC) {
		// compressed file
		i32 compressedSize;
		i32 originalSize;
		ubyte* pData;

		MemcpyLE((u8*)&compressedSize, (u8*)decoded.pData+1, 4);
		MemcpyLE((u8*)&originalSize, (u8*)decoded.pData+5, 4);
		pData = (ubyte*)decoded.pData+9;

		u8* pDecompressedData = New(g_pArenaFrame, u8, originalSize);

		LZ4_decompress_safe((char*)pData, (char*)pDecompressedData, compressedSize, originalSize);

		CborParserState state;
		state.len = originalSize;
		state.pData = pDecompressedData;
		state.pEnd = pDecompressedData + state.len;
		state.pCurrent = state.pData;

		ParseCborValue(L, state);
	}
	else {
		// text
		Scan::ScanningState scan;
		scan.pTextStart = decoded.pData;
		scan.pTextEnd = decoded.pData + decoded.length;
		scan.pCurrent = (char*)scan.pTextStart;
		scan.line = 1;

		ParseTextValue(L, scan);
	}

	// parse metadata
	if (metaData.length > 0) {
		Scan::ScanningState scan;
		scan.pTextStart = metaData.pData;
		scan.pTextEnd = metaData.pData + metaData.length;
		scan.pCurrent = (char*)scan.pTextStart;
		scan.line = 1;

		ParseTextTable(L, scan, true);
	}

	return metaData.length > 0 ? 2 : 1;
}

// ***********************************************************************

int Store(lua_State* L) {
	usize filenameLen;
	const char* filename = luaL_checklstring(L, 1, &filenameLen);

	Serialize(L);

	// string is now on the stack
	usize contentLen;
	const char* content = lua_tolstring(L, -1, &contentLen);

	SDL_RWops* pFile = SDL_RWFromFile(filename, "wb");
	SDL_RWwrite(pFile, content, contentLen, 1);
	SDL_RWclose(pFile);
	return 0;
}

// ***********************************************************************

int Load(lua_State* L) {
	usize filenameLen;
	const char* filename = luaL_checklstring(L, 1, &filenameLen);

    SDL_RWops* pFile = SDL_RWFromFile(filename, "rb");

    u64 fileSize = SDL_RWsize(pFile);
    char* pFileContent = New(g_pArenaFrame, char, fileSize);
    SDL_RWread(pFile, pFileContent, fileSize, 1);
    SDL_RWclose(pFile);

	lua_pushlstring(L, pFileContent, fileSize);
	return Deserialize(L);
}

// ***********************************************************************

void BindSerialization(lua_State* L) {
	// register global functions
	const luaL_Reg globalFuncs[] = {
		{ "serialize", Serialize },
		{ "deserialize", Deserialize },
		{ "store", Store },
		{ "load", Load },
		{ NULL, NULL }
	};

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, globalFuncs);
	lua_pop(L, 1);
}

}
