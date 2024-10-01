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

void ParseJsonNodeRecursively(lua_State* L, JsonValue& gltf, JsonValue& nodeToParse) {
	String nodeName = nodeToParse.HasKey("name") ? nodeToParse["name"].ToString() : String("");
	lua_pushlstring(L, nodeName.pData, nodeName.length);

	// table for the node itself
	lua_newtable(L);

	// position
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
	if (nodeToParse.HasKey("rotation")) {
		lua_newtable(L);
		lua_pushnumber(L, nodeToParse["rotation"][0].ToFloat());
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, nodeToParse["rotation"][1].ToFloat());
		lua_setfield(L, -2, "y");
		lua_pushnumber(L, nodeToParse["rotation"][2].ToFloat());
		lua_setfield(L, -2, "z");
		lua_pushnumber(L, nodeToParse["rotation"][3].ToFloat());
		lua_setfield(L, -2, "w");
		lua_setfield(L, -2, "rotation");
	}

	// scale
	if (nodeToParse.HasKey("scale")) {
		lua_newtable(L);
		lua_pushnumber(L, nodeToParse["scale"][0].ToFloat());
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, nodeToParse["scale"][1].ToFloat());
		lua_setfield(L, -2, "y");
		lua_pushnumber(L, nodeToParse["scale"][2].ToFloat());
		lua_setfield(L, -2, "z");
		lua_setfield(L, -2, "scale");
	}

	// mesh identifier
	if (nodeToParse.HasKey("mesh")) {
		i32 meshId = nodeToParse["mesh"].ToInt();
		String meshName = gltf["meshes"][meshId]["name"].ToString();
		lua_pushlstring(L, meshName.pData, meshName.length);
		lua_setfield(L, -2, "mesh");
	}

	// children
	if (nodeToParse.HasKey("children")) {
		lua_newtable(L);
		i32 childCount = nodeToParse["children"].Count();
		for (int i = 0; i < childCount; i++) {
			i32 childId = nodeToParse["children"][i].ToInt();
			ParseJsonNodeRecursively(L, gltf, gltf["nodes"][childId]);
		}
		lua_setfield(L, -2, "children");
	}

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

	// note that the nodelist in the top level scene does not include child nodes
	// so they are not included in this list, but will be parsed recursively
	JsonValue& topLevelNodeList = parsed["scenes"][0]["nodes"];
	i32 topLevelNodesCount = topLevelNodeList.Count();
    for (int i = 0; i < topLevelNodesCount; i++) {
		i32 nodeId = topLevelNodeList[i].ToInt();
		JsonValue& node = parsed["nodes"][nodeId];
		ParseJsonNodeRecursively(L, parsed, node);
	}

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
