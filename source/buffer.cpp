// Copyright David Colson. All rights reserved.
#include "buffer.h"

#include <lua.h>
#include <lualib.h>
#include <memory.h>
#include <string.h>
#include <maths.h>

namespace BufferLib {


// ***********************************************************************

void SetImpl(lua_State* L, Buffer* pBuffer, i32 index, i32 startParam) {
	// now grab as many integers as you can find and put them in the buffer from the starting index
	// while until none or nil
	i32 paramCounter = startParam;
	switch(pBuffer->type) {
		case Type::Float32: {
			f32* pData = (f32*)pBuffer->pData;
			pData += index;
			while (lua_isnumber(L, paramCounter) == 1) {
				*pData = (f32)lua_tonumber(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Int32: {
			i32* pData = (i32*)pBuffer->pData;
			pData += index;
			while (lua_isnumber(L, paramCounter) == 1) {
				*pData = (i32)lua_tointeger(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Int16: {
			i16* pData = (i16*)pBuffer->pData;
			pData += index;
			while (lua_isnumber(L, paramCounter) == 1) {
				*pData = (i16)lua_tointeger(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Uint8: {
			u8* pData = (u8*)pBuffer->pData;
			pData += index;
			while (lua_isnumber(L, paramCounter) == 1) {
				*pData = (u8)lua_tounsigned(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
	}
}

// ***********************************************************************

i32 GetImpl(lua_State* L, Buffer* pBuffer, i32 index, i32 count) {
	switch(pBuffer->type) {
		case Type::Float32: {
			f32* pData = (f32*)pBuffer->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushnumber(L, pData[i]);
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

// ***********************************************************************

i32 NewBufferImpl(lua_State* L, const char* type, i32 width, i32 height) {
	Buffer* pBuffer = (Buffer*)lua_newuserdatadtor(L, sizeof(Buffer), [](void* pData) {
		Buffer* pBuffer = (Buffer*)pData;
		if (pBuffer) {
			g_Allocator.Free(pBuffer->pData);
		}
	});

	i32 typeSize = 0;
	if (strcmp(type, "f32") == 0) {
		pBuffer->type = Type::Float32;
		typeSize = sizeof(f32);
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

// ***********************************************************************

i32 NewBuffer(lua_State* L) {
    usize len;
    const char* type = luaL_checklstring(L, 1, &len);
    i32 width = (i32)luaL_checkinteger(L, 2);
	i32 height = 1;
	if (lua_isnoneornil(L, 3) == 0) {
		height = (i32)luaL_checkinteger(L, 3);
	}
	return NewBufferImpl(L, type, width, height);
}

// ***********************************************************************

i32 NewVec(lua_State* L) {
	i32 nArgs = lua_gettop(L);
	NewBufferImpl(L, "f32", 3, 1);
    Buffer* pBuffer = (Buffer*)lua_touserdata(L, -1);
	if (nArgs > 0)
		SetImpl(L, pBuffer, 0, 1);
	return 1;
}

// ***********************************************************************

i32 Set(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
    i32 index = (i32)luaL_checkinteger(L, 2);
	SetImpl(L, pBuffer, index, 3);
	return 0;
}

// ***********************************************************************

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

// ***********************************************************************

i32 Get(lua_State* L) {
	//TODO: bounds checking
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
    i32 index = (i32)luaL_checkinteger(L, 2);
	i32 count = (i32)luaL_checkinteger(L, 3);
	return GetImpl(L, pBuffer, index, count);
}

// ***********************************************************************

i32 Get2D(lua_State* L) {
	//TODO: bounds checking
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

// ***********************************************************************

i32 Index(lua_State* L) {
	// first we have the userdata
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");

	// if key is number it's an array index read, otherwise it's text
	if (lua_isnumber(L, 2)) {
		return GetImpl(L, pBuffer, lua_tointeger(L, 2), 1);
	}

    usize len;
    const char* str = luaL_checklstring(L, 2, &len);

	i32 index = -1;
	if (strcmp(str, "x") == 0) { index = 0; }
	else if (strcmp(str, "y") == 0) { index = 1; }
	else if (strcmp(str, "z") == 0) { index = 2; }
	else if (strcmp(str, "w") == 0) { index = 3; }

	if (index >= 0) {
		return GetImpl(L, pBuffer, index, 1);
	}
	else {
		// standard indexing behaviour
		luaL_getmetatable(L, "Buffer");
		lua_getfield(L, -1, str);
		return 1;
	}
}

// ***********************************************************************

i32 NewIndex(lua_State* L) {
	// first we have the userdata
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");

	if (lua_isnumber(L, 2)) {
		SetImpl(L, pBuffer, lua_tointeger(L, 2), 3);
	}

    usize len;
    const char* str = luaL_checklstring(L, 2, &len);

	i32 index = -1;
	if (strcmp(str, "x") == 0) { index = 0; }
	else if (strcmp(str, "y") == 0) { index = 1; }
	else if (strcmp(str, "z") == 0) { index = 2; }
	else if (strcmp(str, "w") == 0) { index = 3; }

	if (index >= 0) {
		SetImpl(L, pBuffer, index, 3);
	}
	else {
		// standard indexing behaviour
		luaL_getmetatable(L, "Buffer");
		lua_setfield(L, -1, str);
		return 1;
	}
	return 0;
}

// ***********************************************************************

i32 Width(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
	lua_pushinteger(L, pBuffer->width);
	return 1;
}

// ***********************************************************************

i32 Height(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
	lua_pushinteger(L, pBuffer->height);
	return 1;
}

// ***********************************************************************

i32 Size(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
	lua_pushinteger(L, pBuffer->width * pBuffer->height);
	return 1;
}

// ***********************************************************************

#define OPERATOR_FUNC(op, name) 											\
i32 name(lua_State* L) {													\
    Buffer* pBuffer1 = (Buffer*)luaL_checkudata(L, 1, "Buffer");			\
    Buffer* pBuffer2 = (Buffer*)luaL_checkudata(L, 2, "Buffer");			\
																			\
	if (pBuffer1->type != pBuffer2->type) {									\
		luaL_error(L, "Type mismatch in buffer operation");					\
	}																		\
	i32 buf1Size = pBuffer1->width * pBuffer1->height;						\
	i32 buf2Size = pBuffer2->width * pBuffer2->height;						\
	i32 resultSize = min(buf1Size, buf2Size);								\
																			\
	const char* typeStr = "f32";											\
	switch(pBuffer1->type) {												\
		case Type::Float32: typeStr = "f32"; break;							\
		case Type::Int32: typeStr = "i32"; break;							\
		case Type::Int16: typeStr = "i16"; break;							\
		case Type::Uint8: typeStr = "u8"; break;							\
	}																		\
	NewBufferImpl(L, typeStr, pBuffer1->width, pBuffer1->height);			\
    Buffer* pBuffer = (Buffer*)lua_touserdata(L, -1);						\
																			\
	switch(pBuffer->type) {													\
		case Type::Float32: {												\
			f32* pResult = (f32*)pBuffer->pData;							\
			f32* pBuf1 = (f32*)pBuffer1->pData;								\
			f32* pBuf2 = (f32*)pBuffer2->pData;								\
			for (int i=0; i < resultSize; i++) 								\
				pResult[i] = pBuf1[i] op pBuf2[i];							\
			break;															\
		}																	\
		case Type::Int32: {													\
			i32* pResult = (i32*)pBuffer->pData;							\
			i32* pBuf1 = (i32*)pBuffer1->pData;								\
			i32* pBuf2 = (i32*)pBuffer2->pData;								\
			for (int i=0; i < resultSize; i++)								\
				pResult[i] = pBuf1[i] op pBuf2[i];							\
			break;															\
		}																	\
		case Type::Int16: {													\
			i16* pResult = (i16*)pBuffer->pData;							\
			i16* pBuf1 = (i16*)pBuffer1->pData;								\
			i16* pBuf2 = (i16*)pBuffer2->pData;								\
			for (int i=0; i < resultSize; i++) 								\
				pResult[i] = pBuf1[i] op pBuf2[i];							\
			break;															\
		}																	\
		case Type::Uint8: {													\
			u8* pResult = (u8*)pBuffer->pData;								\
			u8* pBuf1 = (u8*)pBuffer1->pData;								\
			u8* pBuf2 = (u8*)pBuffer2->pData;								\
			for (int i=0; i < resultSize; i++) 								\
				pResult[i] = pBuf1[i] op pBuf2[i];							\
			break;															\
		}																	\
	}																		\
	return 1;																\
}																			

OPERATOR_FUNC(+, Add)
OPERATOR_FUNC(-, Sub)
OPERATOR_FUNC(*, Mul)
OPERATOR_FUNC(/, Div)

// ***********************************************************************

template<typename T>
T CalcMagnitude(Buffer* pBuffer) {
	T sum = T();
	i32 bufSize = pBuffer->width * pBuffer->height;
	for (i32 i = 0; i < bufSize; i++) {
		T* pData = (T*)pBuffer->pData;
		T elem = pData[i];
		sum += elem*elem;
	}
	return sqrt(sum);
}

// ***********************************************************************

i32 VecMagnitude(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
	switch(pBuffer->type) {
		case Type::Float32: lua_pushnumber(L, CalcMagnitude<f32>(pBuffer)); break;
		case Type::Int32: lua_pushinteger(L, CalcMagnitude<i32>(pBuffer)); break;
		case Type::Int16: lua_pushinteger(L, CalcMagnitude<i16>(pBuffer)); break;
		case Type::Uint8: lua_pushunsigned(L, CalcMagnitude<u8>(pBuffer)); break;
		default: return 0;
	}
	return 1;
}

// ***********************************************************************

template<typename T>
T CalcDistance(Buffer* pBuffer, Buffer* pOther) {
	T sum = T();
	i32 bufSize = pBuffer->width * pBuffer->height;
	for (i32 i = 0; i < bufSize; i++) {
		T* pData = (T*)pBuffer->pData;
		T* pOtherData = (T*)pOther->pData;
		T diff = pOtherData[i] - pData[i];
		sum += diff*diff;
	}
	return sqrt(sum);
}

// ***********************************************************************

i32 VecDistance(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
    Buffer* pOther = (Buffer*)luaL_checkudata(L, 2, "Buffer");

	i32 bufSize = pBuffer->width * pBuffer->height;
	i32 otherSize = pBuffer->width * pBuffer->height;
	if (bufSize != otherSize) {
		luaL_error(L, "Both buffers must be the same size for Distance");	
	}

	switch(pBuffer->type) {
		case Type::Float32: lua_pushnumber(L, CalcDistance<f32>(pBuffer, pOther)); break;
		case Type::Int32: lua_pushinteger(L, CalcDistance<i32>(pBuffer, pOther)); break;
		case Type::Int16: lua_pushinteger(L, CalcDistance<i16>(pBuffer, pOther)); break;
		case Type::Uint8: lua_pushunsigned(L, CalcDistance<u8>(pBuffer, pOther)); break;
		default: return 0;
	}
	return 1;
}

// ***********************************************************************

template<typename T>
T CalcDot(Buffer* pBuffer, Buffer* pOther) {
	T sum = T();
	i32 bufSize = pBuffer->width * pBuffer->height;
	for (i32 i = 0; i < bufSize; i++) {
		T* pData = (T*)pBuffer->pData;
		T* pOtherData = (T*)pOther->pData;
		sum += pOtherData[i] * pData[i];
	}
	return sum;
}


// ***********************************************************************

i32 VecDot(lua_State* L) {
    Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");
    Buffer* pOther = (Buffer*)luaL_checkudata(L, 2, "Buffer");

	i32 bufSize = pBuffer->width * pBuffer->height;
	i32 otherSize = pBuffer->width * pBuffer->height;
	if (bufSize != otherSize) {
		luaL_error(L, "Both buffers must be the same size for Dot");	
	}

	switch(pBuffer->type) {
		case Type::Float32: lua_pushnumber(L, CalcDot<f32>(pBuffer, pOther)); break;
		case Type::Int32: lua_pushinteger(L,  CalcDot<i32>(pBuffer, pOther)); break;
		case Type::Int16: lua_pushinteger(L,  CalcDot<i16>(pBuffer, pOther)); break;
		case Type::Uint8: lua_pushunsigned(L, CalcDot<u8>(pBuffer, pOther)); break;
		default: return 0;
	}
	return 1;
}
 
// ***********************************************************************

void BindBuffer(lua_State* L) {

	// register global functions
    const luaL_Reg globalFuncs[] = {
        { "NewBuffer", NewBuffer },
        { "NewVec", NewVec },
        { NULL, NULL }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, NULL, globalFuncs);
    lua_pop(L, 1);

	// new metatable
	luaL_newmetatable(L, "Buffer");

	// register metamethods
    const luaL_Reg bufferMethods[] = {
        { "__index", Index },
        { "__newindex", NewIndex },
        { "__add", Add },
        { "__sub", Sub },
        { "__mul", Mul },
        { "__div", Div },
        { "Set", Set },
        { "Set2D", Set2D },
        { "Get", Get },
        { "Get2D", Get2D },
        { "Width", Width },
        { "Height", Height },
        { "Size", Size },
        { "Magnitude", VecMagnitude },
        { "Distance", VecDistance },
        { "Dot", VecDot },
        { NULL, NULL }
    };

	// TODO: some future functionality that might be useful:
	// Cross product
	// more fancy operations such as using strides, offsets, scalar against buffer etc
	// helpers for treating buffers as matrices, matmul functions, transpose, inverse etc

	luaL_register(L, NULL, bufferMethods);
    lua_pop(L, 1);
}

}