// Copyright David Colson. All rights reserved.

// ***********************************************************************

bool VFSPathToRealPath(String vfsPath, String& outRealPath, Arena* pArena) {
	StringBuilder builder(g_pArenaFrame);
	builder.Append("system/");

	// absolute path, so map the mount point to a real path
	if (vfsPath[0] == '/') {
		if (strncmp(vfsPath.pData, "/shared", 7) == 0) {
			builder.Append(vfsPath);
		}
		else if (strncmp(vfsPath.pData, "/app", 4) == 0) { 
			builder.Append(Cpu::GetAppName());
			builder.Append(vfsPath.pData + 4);
		}
		else {
			return false;
		}
	}
	// relative path, so make it relative to the app's location
	else {
		builder.Append(Cpu::GetAppName());
		builder.Append("/");
		builder.Append(vfsPath);
	}

	outRealPath = builder.CreateString(pArena);
	return true;
}

// ***********************************************************************

int VfsStore(lua_State* L) {
	u64 filenameLen;
	const char* filename = luaL_checklstring(L, 1, &filenameLen);

	String realFileName; 
	if (!VFSPathToRealPath(String(filename), realFileName, g_pArenaFrame)) {
		luaL_error(L, "Filepath not in a valid mount point", filename);
		return 0;
	}

	Serialize(L);

	// string is now on the stack
	u64 contentLen;
	const char* content = lua_tolstring(L, -1, &contentLen);

	WriteWholeFile(realFileName, content, contentLen);
	return 0;
}

// ***********************************************************************

int VfsLoad(lua_State* L) {
	u64 filenameLen;
	const char* filename = luaL_checklstring(L, 1, &filenameLen);

	String realFileName; 
	if (!VFSPathToRealPath(String(filename), realFileName, g_pArenaFrame)) {
		luaL_error(L, "Filepath not in a valid mount point", filename);
		return 0;
	}

    i64 fileSize;
    char* pFileContent = ReadWholeFile(realFileName, &fileSize, g_pArenaFrame);
	if (pFileContent == nullptr) {
		luaL_error(L, "Failed to load file %s", filename);
		return 0;
	}

	lua_pushlstring(L, pFileContent, fileSize);
	return Deserialize(L);
}

// ***********************************************************************

int VfsMakeDirectory(lua_State* L) {
	u64 filenameLen;
	const char* filename = luaL_checklstring(L, 1, &filenameLen);
	bool makeAll = (bool)luaL_checkboolean(L, 2);

	String realFileName; 
	if (!VFSPathToRealPath(String(filename), realFileName, g_pArenaFrame)) {
		luaL_error(L, "Filepath not in a valid mount point", filename);
		lua_pushboolean(L, false);
		return 1;
	}

	bool result = MakeDirectory(realFileName, makeAll);
	lua_pushboolean(L, result);
	return 1;
}

// ***********************************************************************

int VfsRemove(lua_State* L) {
	u64 filenameLen;
	const char* filename = luaL_checklstring(L, 1, &filenameLen);

	String realFileName; 
	if (!VFSPathToRealPath(String(filename), realFileName, g_pArenaFrame)) {
		luaL_error(L, "Filepath not in a valid mount point", filename);
		lua_pushboolean(L, false);
		return 1;
	}

	bool result = RemoveFileOrDirectory(realFileName);
	lua_pushboolean(L, result);
	return 1;
}

// ***********************************************************************

int VfsCopy(lua_State* L) {
	u64 fromLen;
	const char* from = luaL_checklstring(L, 1, &fromLen);
	String realFrom; 
	if (!VFSPathToRealPath(String(from), realFrom, g_pArenaFrame)) {
		luaL_error(L, "Filepath not in a valid mount point", from);
		lua_pushboolean(L, false);
		return 1;
	}

	u64 toLen;
	const char* to = luaL_checklstring(L, 2, &toLen);
	String realTo; 
	if (!VFSPathToRealPath(String(to), realTo, g_pArenaFrame)) {
		luaL_error(L, "Filepath not in a valid mount point", to);
		lua_pushboolean(L, false);
		return 1;
	}


	bool result = CopyFile(realFrom, realTo);
	lua_pushboolean(L, result);
	return 1;
}

// ***********************************************************************

int VfsMove(lua_State* L) {
	u64 fromLen;
	const char* from = luaL_checklstring(L, 1, &fromLen);
	String realFrom; 
	if (!VFSPathToRealPath(String(from), realFrom, g_pArenaFrame)) {
		luaL_error(L, "Filepath not in a valid mount point", from);
		lua_pushboolean(L, false);
		return 1;
	}

	u64 toLen;
	const char* to = luaL_checklstring(L, 2, &toLen);
	String realTo; 
	if (!VFSPathToRealPath(String(to), realTo, g_pArenaFrame)) {
		luaL_error(L, "Filepath not in a valid mount point", to);
		lua_pushboolean(L, false);
		return 1;
	}


	bool result = MoveFile(realFrom, realTo);
	lua_pushboolean(L, result);
	return 1;
}
// ***********************************************************************

void BindFileSystem(lua_State* L) {

	const luaL_Reg globalFuncs[] = {
		{ "store", VfsStore },
		{ "load", VfsLoad },
		{ "make_directory", VfsMakeDirectory },
		{ "remove", VfsRemove },
		{ "copy", VfsCopy },
		{ "move", VfsMove },
		// { "list_directory", VfsListDirectory },
		{ NULL, NULL }
	};

	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, globalFuncs);
	lua_pop(L, 1);
}
