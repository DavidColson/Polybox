// Copyright David Colson. All rights reserved.

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

Buffer* AllocBuffer(lua_State* L, Type type, i32 width, i32 height) {
	i32 typeSize = 0;
	switch (type) {
		case Type::Float32: typeSize = sizeof(f32); break;
		case Type::Int32: typeSize = sizeof(i32); break;
		case Type::Int16: typeSize = sizeof(i16); break;
		case Type::Uint8: typeSize = sizeof(u8); break;
	}

	i32 bufSize = width * height * typeSize;
	Buffer* pBuffer = (Buffer*)lua_newuserdata(L, sizeof(Buffer) + bufSize);
	pBuffer->pData = (u8*)pBuffer + sizeof(Buffer);
	pBuffer->width = width;
	pBuffer->height = height;
	pBuffer->type = type;
	pBuffer->img.id = SG_INVALID_ID;
	pBuffer->dirty = false;
	pBuffer->dynamic = false;
 
	luaL_getmetatable(L, "Buffer");
	lua_setmetatable(L, -2);
	return pBuffer;
}

// ***********************************************************************

i64 GetBufferSize(Buffer* pBuffer) {
	i32 typeSize = 0;
	switch (pBuffer->type) {
		case Type::Float32: typeSize = sizeof(f32); break;
		case Type::Int32: typeSize = sizeof(i32); break;
		case Type::Int16: typeSize = sizeof(i16); break;
		case Type::Uint8: typeSize = sizeof(u8); break;
	}

	return pBuffer->width * pBuffer->height * typeSize;
}

// ***********************************************************************

void UpdateBufferImage(Buffer* pBuffer) {
	// @todo: error if buffer type is not suitable for image data
	// i.e. must be int32 etc and 2D

	if (pBuffer->img.id == SG_INVALID_ID ||
		(pBuffer->dirty && pBuffer->dynamic == false))
	{
		sg_image_desc imageDesc = {
			.width = pBuffer->width,
			.height = pBuffer->height,
			.pixel_format = SG_PIXELFORMAT_RGBA8,
		};

		// make dynamic if someone is editing this
		// image after creation
		pBuffer->dynamic = false;
		if (pBuffer->img.id != SG_INVALID_ID) {
			sg_destroy_image(pBuffer->img);
			imageDesc.usage = SG_USAGE_STREAM;
			pBuffer->dynamic = true;
		}

		sg_range pixelsData;
		pixelsData.ptr = (void*)pBuffer->pData;
		pixelsData.size = pBuffer->width * pBuffer->height * 4 * sizeof(u8);
		imageDesc.data.subimage[0][0] = pixelsData;
		pBuffer->img = sg_make_image(&imageDesc);
		pBuffer->dirty = false;
	}

	// @todo: only allow one edit per frame, somehow block this
	if (pBuffer->dirty && pBuffer->dynamic) {
		// already dynamic, so we can just update it
		sg_range range;
		range.ptr = (void*)pBuffer->pData;
		range.size = pBuffer->width * pBuffer->height * 4 * sizeof(u8);
		sg_image_data data;
		data.subimage[0][0] = range;
		sg_update_image(pBuffer->img, data);  
	}

}

// ***********************************************************************

void ParseBufferDataString(lua_State* L, String dataString, Buffer* pBuffer) {
	if (dataString.length <= 0) 
		return;

	Scan::ScanningState scan;
	scan.pTextStart = dataString.pData;
	scan.pTextEnd = dataString.pData + dataString.length;
	scan.pCurrent = (char*)dataString.pData;
	scan.line = 1;

	switch(pBuffer->type) {
		case BufferLib::Type::Float32: {
			f32* pData = (f32*)pBuffer->pData;
			while (!Scan::IsAtEnd(scan)) {
				scan.pCurrent++; // parse number expects that the first digit has been parsed
				*pData = ParseNumber(scan);
				if (!Scan::IsAtEnd(scan) && Scan::Peek(scan) != ',')
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
			while(!Scan::IsAtEnd(scan)) {
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
			while (!Scan::IsAtEnd(scan)) {
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
			while (!Scan::IsAtEnd(scan)) {
				number[0] = *scan.pCurrent++;
				number[1] = *scan.pCurrent++;
				*pData = (u8)strtol(number, nullptr, 16);
				pData++;
			}
			Scan::Advance(scan);
			break;
		}
	}
}

// ***********************************************************************

i32 NewBuffer(lua_State* L) {
	i32 nArgs = lua_gettop(L);
    const char* typeStr = luaL_checkstring(L, 1);
    i32 width = (i32)luaL_checkinteger(L, 2);
	i32 height = 1;
	String dataStr;

	if (nArgs > 2) {
		if (lua_isnumber(L, 3)) {
			height = (i32)lua_tointeger(L, 3);
		}
		else if (lua_isstring(L, 3)) {
			dataStr = lua_tostring(L, 3);
		}
		else {
			luaL_error(L, "Unexpected 3rd argument to buffer, should be integer or string");
		}
	}

	if (nArgs > 3) {
		u64 len;
		dataStr.pData = (char*)luaL_checklstring(L, 4, &len);
		dataStr.length = (i64)len;
	}

	Type type;
	if (strcmp(typeStr, "f32") == 0) {
		type = Type::Float32;
	}
	else if (strcmp(typeStr, "i32") == 0) {
		type = Type::Int32;
	}
	else if (strcmp(typeStr, "i16") == 0) {
		type = Type::Int16;
	}
	else if (strcmp(typeStr, "u8") == 0) {
		type = Type::Uint8;
	}
	else {
		luaL_error(L, "invalid type given to buffer creation %s", typeStr);
		return 0;
	}
	Buffer* pBuffer = AllocBuffer(L, type, width, height);

	if (dataStr.length > 0) {
		ParseBufferDataString(L, dataStr, pBuffer); 
	}
	return 1;
}

// ***********************************************************************

i32 NewVec(lua_State* L) {
	i32 nArgs = lua_gettop(L);
	Buffer* pBuffer = AllocBuffer(L, Type::Float32, 3, 1);
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

    u64 len;
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

    u64 len;
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

i32 ToString(lua_State* L) {
	Buffer* pBuffer = (Buffer*)luaL_checkudata(L, 1, "Buffer");

	StringBuilder builder(g_pArenaFrame);

	switch(pBuffer->type) {
		case BufferLib::Type::Float32: builder.Append("buffer(\"f32\""); break;
		case BufferLib::Type::Int32: builder.Append("buffer(\"i32\""); break;
		case BufferLib::Type::Int16: builder.Append("buffer(\"i16\""); break;
		case BufferLib::Type::Uint8: builder.Append("buffer(\"u8\"");; break;
	}
	builder.AppendFormat(",%i,%i,\"", pBuffer->width, pBuffer->height);
	switch(pBuffer->type) {
		case BufferLib::Type::Float32: {
			f32* pFloats = (f32*)pBuffer->pData;
			for (int i = 0; i < pBuffer->width*pBuffer->height; i++) {
				if (i+1 < pBuffer->width*pBuffer->height)
					builder.AppendFormat("%.9g,", pFloats[i]);
				else
					builder.AppendFormat("%.9g", pFloats[i]);
			}
			break;
		}
		case BufferLib::Type::Int32: {
			// 8 hex digits
			i32* pInts = (i32*)pBuffer->pData;
			for (int i = 0; i < pBuffer->width*pBuffer->height; i++)
				builder.AppendFormat("%08x", pInts[i]);
			break;
		}
		case BufferLib::Type::Int16: {
			// 4 hex digits
			i16* pInts = (i16*)pBuffer->pData;
			for (int i = 0; i < pBuffer->width*pBuffer->height; i++)
				builder.AppendFormat("%04x", pInts[i]);
			break;
		}
		case BufferLib::Type::Uint8: {
			// two hex digits == 1 byte
			u8* pInts = (u8*)pBuffer->pData;
			for (int i = 0; i < pBuffer->width*pBuffer->height; i++)
				builder.AppendFormat("%02x", pInts[i]);
			break;
		}
	}
	builder.Append("\")");

	String result = builder.CreateString(g_pArenaFrame);
	lua_pushstring(L, result.pData);
	return 1;
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
	Buffer* pBuffer = AllocBuffer(L, pBuffer1->type, pBuffer1->width, pBuffer1->height);\
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
        { "buffer", NewBuffer },
        { "vec", NewVec },
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
		{ "__tostring", ToString },
        { "__add", Add },
        { "__sub", Sub },
        { "__mul", Mul },
        { "__div", Div },
        { "set", Set },
        { "set2D", Set2D },
        { "get", Get },
        { "get2D", Get2D },
        { "width", Width },
        { "height", Height },
        { "size", Size },
        { "magnitude", VecMagnitude },
        { "distance", VecDistance },
        { "dot", VecDot },
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
