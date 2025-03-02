// Copyright David Colson. All rights reserved.
#pragma once

struct lua_State;
struct String;

// Considering renaming to userdata, it's just better
namespace BufferLib {
 
enum class Type:u8 {
	Float32,
	Int32,
	Int16,
	Uint8
};

struct Buffer {
	i32 width;
	i32 height;
	Type type;
	u8* pData;
};

Buffer* AllocBuffer(lua_State* L, Type type, i32 width, i32 height);
i64 GetBufferSize(Buffer* pBuffer);
void ParseBufferDataString(lua_State* L, String dataString, Buffer* pBuffer);
void BindBuffer(lua_State* L);
}
