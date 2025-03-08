// Copyright David Colson. All rights reserved.


// ***********************************************************************

void SetImpl(lua_State* L, UserData* pUserData, i32 index, i32 startParam) {
	// now grab as many integers as you can find and put them in the userdata from the starting index
	// while until none or nil
	i32 paramCounter = startParam;
	switch(pUserData->type) {
		case Type::Float32: {
			f32* pData = (f32*)pUserData->pData;
			pData += index;
			while (lua_isnumber(L, paramCounter) == 1) {
				*pData = (f32)lua_tonumber(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Int32: {
			i32* pData = (i32*)pUserData->pData;
			pData += index;
			while (lua_isnumber(L, paramCounter) == 1) {
				*pData = (i32)lua_tointeger(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Int16: {  
			i16* pData = (i16*)pUserData->pData;
			pData += index;
			while (lua_isnumber(L, paramCounter) == 1) {
				*pData = (i16)lua_tointeger(L, paramCounter);			
				pData++; paramCounter++;
			}
			break;
		}
		case Type::Uint8: {
			u8* pData = (u8*)pUserData->pData;
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

i32 GetImpl(lua_State* L, UserData* pUserData, i32 index, i32 count) {
	switch(pUserData->type) {
		case Type::Float32: {
			f32* pData = (f32*)pUserData->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushnumber(L, pData[i]);
			}
			break;
		}
		case Type::Int32: {
			i32* pData = (i32*)pUserData->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushinteger(L, pData[i]);
			}
			break;
		}
		case Type::Int16: {
			i16* pData = (i16*)pUserData->pData;
			pData += index;
			for (i32 i = 0; i < count; i++) {
				lua_pushinteger(L, pData[i]);
			}
			break;
		}
		case Type::Uint8: {
			u8* pData = (u8*)pUserData->pData;
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

UserData* AllocUserData(lua_State* L, Type type, i32 width, i32 height) {
	i32 typeSize = 0;
	switch (type) {
		case Type::Float32: typeSize = sizeof(f32); break;
		case Type::Int32: typeSize = sizeof(i32); break;
		case Type::Int16: typeSize = sizeof(i16); break;
		case Type::Uint8: typeSize = sizeof(u8); break;
	}

	i32 bufSize = width * height * typeSize;
	UserData* pUserData = (UserData*)lua_newuserdata(L, sizeof(UserData) + bufSize);
	pUserData->pData = (u8*)pUserData + sizeof(UserData);
	pUserData->width = width;
	pUserData->height = height;
	pUserData->type = type;
	pUserData->img.id = SG_INVALID_ID;
	pUserData->dirty = false;
	pUserData->dynamic = false;
 
	luaL_getmetatable(L, "UserData");
	lua_setmetatable(L, -2);
	return pUserData;
}

// ***********************************************************************

i64 GetUserDataSize(UserData* pUserData) {
	i32 typeSize = 0;
	switch (pUserData->type) {
		case Type::Float32: typeSize = sizeof(f32); break;
		case Type::Int32: typeSize = sizeof(i32); break;
		case Type::Int16: typeSize = sizeof(i16); break;
		case Type::Uint8: typeSize = sizeof(u8); break;
	}

	return pUserData->width * pUserData->height * typeSize;
}

// ***********************************************************************

void UpdateUserDataImage(UserData* pUserData) {
	// @todo: error if userdata type is not suitable for image data
	// i.e. must be int32 etc and 2D

	if (pUserData->img.id == SG_INVALID_ID ||
		(pUserData->dirty && pUserData->dynamic == false))
	{
		sg_image_desc imageDesc = {
			.width = pUserData->width,
			.height = pUserData->height,
			.pixel_format = SG_PIXELFORMAT_RGBA8,
		};

		// make dynamic if someone is editing this
		// image after creation
		pUserData->dynamic = false;
		if (pUserData->img.id != SG_INVALID_ID) {
			sg_destroy_image(pUserData->img);
			imageDesc.usage = SG_USAGE_STREAM;
			pUserData->dynamic = true;
		}

		sg_range pixelsData;
		pixelsData.ptr = (void*)pUserData->pData;
		pixelsData.size = pUserData->width * pUserData->height * 4 * sizeof(u8);
		imageDesc.data.subimage[0][0] = pixelsData;
		pUserData->img = sg_make_image(&imageDesc);
		pUserData->dirty = false;
	}

	// @todo: only allow one edit per frame, somehow block this
	if (pUserData->dirty && pUserData->dynamic) {
		// already dynamic, so we can just update it
		sg_range range;
		range.ptr = (void*)pUserData->pData;
		range.size = pUserData->width * pUserData->height * 4 * sizeof(u8);
		sg_image_data data;
		data.subimage[0][0] = range;
		sg_update_image(pUserData->img, data);  
	}

}

// ***********************************************************************

void ParseUserDataDataString(lua_State* L, String dataString, UserData* pUserData) {
	if (dataString.length <= 0) 
		return;

	Scan::ScanningState scan;
	scan.pTextStart = dataString.pData;
	scan.pTextEnd = dataString.pData + dataString.length;
	scan.pCurrent = (char*)dataString.pData;
	scan.line = 1;

	switch(pUserData->type) {
		case Type::Float32: {
			f32* pData = (f32*)pUserData->pData;
			while (!Scan::IsAtEnd(scan)) {
				scan.pCurrent++; // parse number expects that the first digit has been parsed
				*pData = (f32)ParseNumber(scan);
				if (!Scan::IsAtEnd(scan) && Scan::Peek(scan) != ',')
					luaL_error(L, "Expected ',' between values");
				else
					Scan::Advance(scan);
				pData++;
			}
			break;
		}
		case Type::Int32: {
			char number[9];
			number[8] = 0;
			i32* pData = (i32*)pUserData->pData;
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
		case Type::Int16: {
			char number[5];
			number[4] = 0;
			i16* pData = (i16*)pUserData->pData;
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
		case Type::Uint8: {
			char number[3];
			number[2] = 0;
			u8* pData = (u8*)pUserData->pData;
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

i32 NewUserData(lua_State* L) {
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
			luaL_error(L, "Unexpected 3rd argument to userdata, should be integer or string");
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
		luaL_error(L, "invalid type given to userdata creation %s", typeStr);
		return 0;
	}
	UserData* pUserData = AllocUserData(L, type, width, height);

	if (dataStr.length > 0) {
		ParseUserDataDataString(L, dataStr, pUserData); 
	}
	return 1;
}

// ***********************************************************************

i32 NewVec(lua_State* L) {
	i32 nArgs = lua_gettop(L);
	UserData* pUserData = AllocUserData(L, Type::Float32, 3, 1);
	if (nArgs > 0)
		SetImpl(L, pUserData, 0, 1);
	return 1;
}

// ***********************************************************************

i32 Set(lua_State* L) {
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
    i32 index = (i32)luaL_checkinteger(L, 2);
	SetImpl(L, pUserData, index, 3);
	return 0;
}

// ***********************************************************************

i32 Set2D(lua_State* L) {
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
	if (pUserData->height == 1) {
		luaL_error(L, "Set2D is only valid on 2-dimensional userdatas");
		return 0;
	}

    i32 x = (i32)luaL_checkinteger(L, 2);
    i32 y = (i32)luaL_checkinteger(L, 3);
	SetImpl(L, pUserData, pUserData->width * y + x, 4);
	return 0;
}

// ***********************************************************************

i32 Get(lua_State* L) {
	//TODO: bounds checking
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
    i32 index = (i32)luaL_checkinteger(L, 2);
	i32 count = (i32)luaL_checkinteger(L, 3);
	return GetImpl(L, pUserData, index, count);
}

// ***********************************************************************

i32 Get2D(lua_State* L) {
	//TODO: bounds checking
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
	if (pUserData->height == 1) {
		luaL_error(L, "Get2D is only valid on 2-dimensional userdatas");
		return 0;
	}
    i32 x = (i32)luaL_checkinteger(L, 2);
    i32 y = (i32)luaL_checkinteger(L, 3);
	i32 count = (i32)luaL_checkinteger(L, 3);
	return GetImpl(L, pUserData, pUserData->width * y + x, count);
}

// ***********************************************************************

i32 Index(lua_State* L) {
	// first we have the userdata
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");

	// if key is number it's an array index read, otherwise it's text
	if (lua_isnumber(L, 2)) {
		return GetImpl(L, pUserData, lua_tointeger(L, 2), 1);
	}

    u64 len;
    const char* str = luaL_checklstring(L, 2, &len);

	i32 index = -1;
	if (strcmp(str, "x") == 0) { index = 0; }
	else if (strcmp(str, "y") == 0) { index = 1; }
	else if (strcmp(str, "z") == 0) { index = 2; }
	else if (strcmp(str, "w") == 0) { index = 3; }

	if (index >= 0) {
		return GetImpl(L, pUserData, index, 1);
	}
	else {
		// standard indexing behaviour
		luaL_getmetatable(L, "UserData");
		lua_getfield(L, -1, str);
		return 1;
	}
}

// ***********************************************************************

i32 NewIndex(lua_State* L) {
	// first we have the userdata
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");

	if (lua_isnumber(L, 2)) {
		SetImpl(L, pUserData, lua_tointeger(L, 2), 3);
	}

    u64 len;
    const char* str = luaL_checklstring(L, 2, &len);

	i32 index = -1;
	if (strcmp(str, "x") == 0) { index = 0; }
	else if (strcmp(str, "y") == 0) { index = 1; }
	else if (strcmp(str, "z") == 0) { index = 2; }
	else if (strcmp(str, "w") == 0) { index = 3; }

	if (index >= 0) {
		SetImpl(L, pUserData, index, 3);
	}
	else {
		// standard indexing behaviour
		luaL_getmetatable(L, "UserData");
		lua_setfield(L, -1, str);
		return 1;
	}
	return 0;
}

// ***********************************************************************

i32 ToString(lua_State* L) {
	UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");

	StringBuilder builder(g_pArenaFrame);

	switch(pUserData->type) {
		case Type::Float32: builder.Append("userdata(\"f32\""); break;
		case Type::Int32: builder.Append("userdata(\"i32\""); break;
		case Type::Int16: builder.Append("userdata(\"i16\""); break;
		case Type::Uint8: builder.Append("userdata(\"u8\"");; break;
	}
	builder.AppendFormat(",%i,%i,\"", pUserData->width, pUserData->height);
	switch(pUserData->type) {
		case Type::Float32: {
			f32* pFloats = (f32*)pUserData->pData;
			for (int i = 0; i < pUserData->width*pUserData->height; i++) {
				if (i+1 < pUserData->width*pUserData->height)
					builder.AppendFormat("%.9g,", pFloats[i]);
				else
					builder.AppendFormat("%.9g", pFloats[i]);
			}
			break;
		}
		case Type::Int32: {
			// 8 hex digits
			i32* pInts = (i32*)pUserData->pData;
			for (int i = 0; i < pUserData->width*pUserData->height; i++)
				builder.AppendFormat("%08x", pInts[i]);
			break;
		}
		case Type::Int16: {
			// 4 hex digits
			i16* pInts = (i16*)pUserData->pData;
			for (int i = 0; i < pUserData->width*pUserData->height; i++)
				builder.AppendFormat("%04x", pInts[i]);
			break;
		}
		case Type::Uint8: {
			// two hex digits == 1 byte
			u8* pInts = (u8*)pUserData->pData;
			for (int i = 0; i < pUserData->width*pUserData->height; i++)
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
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
	lua_pushinteger(L, pUserData->width);
	return 1;
}

// ***********************************************************************

i32 Height(lua_State* L) {
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
	lua_pushinteger(L, pUserData->height);
	return 1;
}

// ***********************************************************************

i32 Size(lua_State* L) {
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
	lua_pushinteger(L, pUserData->width * pUserData->height);
	return 1;
}

// ***********************************************************************

#define OPERATOR_FUNC(op, name) 											\
i32 name(lua_State* L) {													\
    UserData* pUserData1 = (UserData*)luaL_checkudata(L, 1, "UserData");			\
    UserData* pUserData2 = (UserData*)luaL_checkudata(L, 2, "UserData");			\
																			\
	if (pUserData1->type != pUserData2->type) {									\
		luaL_error(L, "Type mismatch in userdata operation");					\
	}																		\
	i32 buf1Size = pUserData1->width * pUserData1->height;						\
	i32 buf2Size = pUserData2->width * pUserData2->height;						\
	i32 resultSize = min(buf1Size, buf2Size);								\
																			\
	UserData* pUserData = AllocUserData(L, pUserData1->type, pUserData1->width, pUserData1->height);\
																			\
	switch(pUserData->type) {													\
		case Type::Float32: {												\
			f32* pResult = (f32*)pUserData->pData;							\
			f32* pBuf1 = (f32*)pUserData1->pData;								\
			f32* pBuf2 = (f32*)pUserData2->pData;								\
			for (int i=0; i < resultSize; i++) 								\
				pResult[i] = pBuf1[i] op pBuf2[i];							\
			break;															\
		}																	\
		case Type::Int32: {													\
			i32* pResult = (i32*)pUserData->pData;							\
			i32* pBuf1 = (i32*)pUserData1->pData;								\
			i32* pBuf2 = (i32*)pUserData2->pData;								\
			for (int i=0; i < resultSize; i++)								\
				pResult[i] = pBuf1[i] op pBuf2[i];							\
			break;															\
		}																	\
		case Type::Int16: {													\
			i16* pResult = (i16*)pUserData->pData;							\
			i16* pBuf1 = (i16*)pUserData1->pData;								\
			i16* pBuf2 = (i16*)pUserData2->pData;								\
			for (int i=0; i < resultSize; i++) 								\
				pResult[i] = pBuf1[i] op pBuf2[i];							\
			break;															\
		}																	\
		case Type::Uint8: {													\
			u8* pResult = (u8*)pUserData->pData;								\
			u8* pBuf1 = (u8*)pUserData1->pData;								\
			u8* pBuf2 = (u8*)pUserData2->pData;								\
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
T CalcMagnitude(UserData* pUserData) {
	T sum = T();
	i32 bufSize = pUserData->width * pUserData->height;
	for (i32 i = 0; i < bufSize; i++) {
		T* pData = (T*)pUserData->pData;
		T elem = pData[i];
		sum += elem*elem;
	}
	return (T)sqrtf((f32)sum);
}

// ***********************************************************************

i32 VecMagnitude(lua_State* L) {
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
	switch(pUserData->type) {
		case Type::Float32: lua_pushnumber(L, CalcMagnitude<f32>(pUserData)); break;
		case Type::Int32: lua_pushinteger(L, CalcMagnitude<i32>(pUserData)); break;
		case Type::Int16: lua_pushinteger(L, CalcMagnitude<i16>(pUserData)); break;
		case Type::Uint8: lua_pushunsigned(L, CalcMagnitude<u8>(pUserData)); break;
		default: return 0;
	}
	return 1;
}

// ***********************************************************************

template<typename T>
T CalcDistance(UserData* pUserData, UserData* pOther) {
	T sum = T();
	i32 bufSize = pUserData->width * pUserData->height;
	for (i32 i = 0; i < bufSize; i++) {
		T* pData = (T*)pUserData->pData;
		T* pOtherData = (T*)pOther->pData;
		T diff = pOtherData[i] - pData[i];
		sum += diff*diff;
	}
	return (T)sqrtf((f32)sum);
}

// ***********************************************************************

i32 VecDistance(lua_State* L) {
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
    UserData* pOther = (UserData*)luaL_checkudata(L, 2, "UserData");

	i32 bufSize = pUserData->width * pUserData->height;
	i32 otherSize = pUserData->width * pUserData->height;
	if (bufSize != otherSize) {
		luaL_error(L, "Both userdatas must be the same size for distance");	
	}

	switch(pUserData->type) {
		case Type::Float32: lua_pushnumber(L, CalcDistance<f32>(pUserData, pOther)); break;
		case Type::Int32: lua_pushinteger(L, CalcDistance<i32>(pUserData, pOther)); break;
		case Type::Int16: lua_pushinteger(L, CalcDistance<i16>(pUserData, pOther)); break;
		case Type::Uint8: lua_pushunsigned(L, CalcDistance<u8>(pUserData, pOther)); break;
		default: return 0;
	}
	return 1;
}

// ***********************************************************************

template<typename T>
T CalcDot(UserData* pUserData, UserData* pOther) {
	T sum = T();
	i32 bufSize = pUserData->width * pUserData->height;
	for (i32 i = 0; i < bufSize; i++) {
		T* pData = (T*)pUserData->pData;
		T* pOtherData = (T*)pOther->pData;
		sum += pOtherData[i] * pData[i];
	}
	return sum;
}


// ***********************************************************************

i32 VecDot(lua_State* L) {
    UserData* pUserData = (UserData*)luaL_checkudata(L, 1, "UserData");
    UserData* pOther = (UserData*)luaL_checkudata(L, 2, "UserData");

	i32 bufSize = pUserData->width * pUserData->height;
	i32 otherSize = pUserData->width * pUserData->height;
	if (bufSize != otherSize) {
		luaL_error(L, "Both userdatas must be the same size for dot");	
	}

	switch(pUserData->type) {
		case Type::Float32: lua_pushnumber(L, CalcDot<f32>(pUserData, pOther)); break;
		case Type::Int32: lua_pushinteger(L,  CalcDot<i32>(pUserData, pOther)); break;
		case Type::Int16: lua_pushinteger(L,  CalcDot<i16>(pUserData, pOther)); break;
		case Type::Uint8: lua_pushunsigned(L, CalcDot<u8>(pUserData, pOther)); break;
		default: return 0;
	}
	return 1;
}
 
// ***********************************************************************

void BindUserData(lua_State* L) {

	// register global functions
    const luaL_Reg globalFuncs[] = {
        { "userdata", NewUserData },
        { "vec", NewVec },
        { NULL, NULL }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, NULL, globalFuncs);
    lua_pop(L, 1);

	// new metatable
	luaL_newmetatable(L, "UserData");

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
