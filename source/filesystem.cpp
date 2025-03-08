// Copyright David Colson. All rights reserved.

// ***********************************************************************

int Store(lua_State* L) {
	u64 filenameLen;
	const char* filename = luaL_checklstring(L, 1, &filenameLen);

	Serialize(L);

	// string is now on the stack
	u64 contentLen;
	const char* content = lua_tolstring(L, -1, &contentLen);

	SDL_RWops* pFile = SDL_RWFromFile(filename, "wb");
	SDL_RWwrite(pFile, content, contentLen, 1);
	SDL_RWclose(pFile);
	return 0;
}

// ***********************************************************************

int Load(lua_State* L) {

	// we expect paths to have two possible mount points
	//   /app and /shared

	// we must parse the pathname given and look for either of the two initial strings
	// if they don't exist, the path is not valid and we fail

	// if they exist then two paths depending on which mount point is requested
	// shared -> add system/ to the start of the path
	// app -> add system/gamename/ to the start of the path

	// then read as normal

	u64 filenameLen;
	const char* filename = luaL_checklstring(L, 1, &filenameLen);

    SDL_RWops* pFile = SDL_RWFromFile(filename, "rb");
	if (pFile == nullptr) {
		luaL_error(L, "Failed to load file %s", filename);
		return 0;
	}

    u64 fileSize = SDL_RWsize(pFile);
    char* pFileContent = New(g_pArenaFrame, char, fileSize);
    SDL_RWread(pFile, pFileContent, fileSize, 1);
    SDL_RWclose(pFile);

	lua_pushlstring(L, pFileContent, fileSize);
	return Deserialize(L);
}

// ***********************************************************************

void BindFileSystem(lua_State* L) {

	const luaL_Reg globalFuncs[] = {
		{ "store", Store },
		{ "load", Load },
		{ NULL, NULL }
	};

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, globalFuncs);
	lua_pop(L, 1);
}
