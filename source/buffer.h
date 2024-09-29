// Copyright David Colson. All rights reserved.

#include <types.h>

struct lua_State;

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
	ubyte* pData;
};

Buffer* AllocBuffer(lua_State* L, Type type, i32 width, i32 height);
void BindBuffer(lua_State* L);
}
