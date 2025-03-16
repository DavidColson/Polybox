// Copyright David Colson. All rights reserved.

struct lua_State;

bool VFSPathToRealPath(String vfsPath, String& outRealPath, Arena* pArena);
void BindFileSystem(lua_State* L);
