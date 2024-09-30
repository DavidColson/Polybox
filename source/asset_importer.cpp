#include "asset_importer.h"

#include "serialization.h"

#include <light_string.h>
#include <lua.h>
#include <SDL_rwops.h>
#include <log.h>
#include <resizable_array.inl>
#include <json.h>

namespace AssetImporter {

// ***********************************************************************

static void* LuaAllocator(void* ud, void* ptr, size_t osize, size_t nsize) {
    if (nsize == 0) {
		RawFree(ptr);
        return nullptr;
    } else {
        return RawRealloc(ptr, nsize, osize, true);
    }
}

// ***********************************************************************

void ParseJsonNodeRecursively(lua_State* L, ResizableArray<i32>& skipList, JsonValue& nodeToParse) {
	String nodeName = nodeToParse.HasKey("name") ? nodeToParse["name"].ToString() : String("");
	lua_pushlstring(L, nodeName.pData, nodeName.length);

	// table for the node itself
	lua_newtable(L);

	// position
	// TODO: may not have position 
	if (nodeToParse.HasKey("translation")) {
		lua_newtable(L);
		lua_pushnumber(L, nodeToParse["translation"][0].ToFloat());
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, nodeToParse["translation"][1].ToFloat());
		lua_setfield(L, -2, "y");
		lua_pushnumber(L, nodeToParse["translation"][2].ToFloat());
		lua_setfield(L, -2, "z");
		lua_setfield(L, -2, "position");
	}

	// rotation
	// scale
	// mesh identifier

	// children

	// set the node element in the table above this i.e. parent.nodename = node
	lua_settable(L, -3);
}

// ***********************************************************************

int Import(Arena* pScratchArena, String source, String output) {
	// we'll do the scene first

	// Load file
	String fileContents;
    SDL_RWops* pFile = SDL_RWFromFile(source.pData, "rb");
	fileContents.length = SDL_RWsize(pFile);
    fileContents.pData = New(pScratchArena, char, fileContents.length);
    SDL_RWread(pFile, fileContents.pData, fileContents.length, 1);
    SDL_RWclose(pFile);

	// ParseJson
    JsonValue parsed = ParseJsonFile(pScratchArena, fileContents);

	// validate gltf
	bool validGltf = parsed["asset"]["version"].ToString() == "2.0";
    if (!validGltf) {
		Log::Info("File %s is not a valid gltf file for us to import", source);
        return 1;
	}

	// Create a lua state, seems a bit odd here I know, but the serialization code is designed to work
	// on lua tables, and so we're just using the lua state to hold those tables, won't actually run any lua code really
	lua_State* L = lua_newstate(LuaAllocator, nullptr);
	Serialization::BindSerialization(L);

	lua_newtable(L);

	// Loop through the list of all nodes flatly, as this is how it's stored in gltf
	ResizableArray<i32> nodesToSkip(pScratchArena);
	i32 totalNodeCount = parsed["nodes"].Count();
    for (int i = 0; i < totalNodeCount; i++) {
		// if a node has children, it will add them to the skip list, and we'll skip them in 
		// this top level for loop
		if (nodesToSkip.Find(i) == nullptr)
			ParseJsonNodeRecursively(L, nodesToSkip, parsed["nodes"][i]);
	}

	// option 1:
	// store list of nodes to skip

	// option 2: 
	// Parse the whole list then go through looking for children to connect

	// table where keys are object names
	// objects have child list with more objects in them
	// i.e. we will NOT flatten the heirarcy as is done in gltf
	// somewhat arbitrary decision, maybe it's wrong I dunno
	// but I feel intuitively it'll be simpler for the user
	// models are stored as strings to their eventual file names

	// once complete output in the desired format, need to make that an argument

	// then the meshes

	// Load the mesh and convert into our interleaved data format, flattening indices
	// merge primitives into one, and give a warning if there are more than one
	// put the final output data into a buffer object in the lua state
	// create a table with format stored in it plus the buffer
	// output a file for each mesh

	return 0;
}

};
