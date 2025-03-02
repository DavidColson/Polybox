namespace AssetImporter {

// ***********************************************************************

static void* LuaAllocator(void* ud, void* ptr, u64 osize, u64 nsize) {
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

// Actually owns the data
struct GltfBuffer {
    char* pBytes { nullptr };
    u64 byteLength { 0 };
};

// Does not actually own the data
struct GltfBufferView {
    // pointer to some place in a buffer
    char* pBuffer { nullptr };
    u64 length { 0 };
};

struct GltfAccessor {
    // pointer to some place in a buffer view
    char* pBuffer { nullptr };
    i32 count { 0 };
    enum ComponentType {
        Byte,
        UByte,
        Short,
        UShort,
        UInt,
        f32
    };
    ComponentType componentType;

    enum Type {
        Scalar,
        Vec2,
        Vec3,
        Vec4,
        Mat2,
        Mat3,
        Mat4
    };
    Type type;
};


// ***********************************************************************

int Import(Arena* pScratchArena, u8 format, String source, String output) {
	// TODO: take the file extension as an indication of what the file is
	// below we are doing gltf only. 

	// if first few bytes is "gltf" in ascii, then it's a binary, glb file, you can verify this with the extension

	// note that if we import a whole folder, we may accidentally import files referenced by the
	// gltf, so when importing seperate gltf files, don't import stuff we already may have picked up, such as images
	// they'll be done individually


	String outputPath = output;
	for (i32 i = outputPath.length-1; i>=0; i--) {
		if (outputPath.pData[i] == '\\' || outputPath.pData[i] == '/') {
			break;
		}
		outputPath.length--;
	}

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
	BufferLib::BindBuffer(L);

	// -------------------------------------
	// Import Scene

	lua_getglobal(L, "serialize");
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
	lua_pushnumber(L, format);
	lua_pcall(L, 2, 1, 0);
	{
		String result;
		size_t len;
		result.pData = (char*)lua_tolstring(L, -1, &len); 
		result.length = (i64)len;

		SDL_RWops* pOutFile = SDL_RWFromFile(output.pData, "wb");
		SDL_RWwrite(pOutFile, result.pData, result.length, 1);
		SDL_RWclose(pOutFile);

	}
	lua_pop(L, 1); // pop the scene table, we're done with it now

	// -------------------------------------
	// Import Meshes

    ResizableArray<GltfBuffer> rawDataBuffers(pScratchArena);
    JsonValue& jsonBuffers = parsed["buffers"];
    for (int i = 0; i < jsonBuffers.Count(); i++) {
        GltfBuffer buf;
        buf.byteLength = jsonBuffers[i]["byteLength"].ToInt();

        String encodedBuffer = jsonBuffers[i]["uri"].ToString();
		// @todo: the substr 37 tells us which type of gltf file this is, glb, bin as separate file, or base 64
		// these should be the options:
		// data:dontcaretext;base64,
			// this of course is what we have here, a base64 encoded string, embedded in the uri
		// filename.bin
			// this will be another file to load, with the binary data directly there
		// no entry other than byte length, then it is glb and the binary is in the next chunk or chunks
        String decoded = DecodeBase64(pScratchArena, encodedBuffer.SubStr(37));
        buf.pBytes = decoded.pData;

        rawDataBuffers.PushBack(buf);
    }

    ResizableArray<GltfBufferView> bufferViews(pScratchArena);
    JsonValue& jsonGltfBufferViews = parsed["bufferViews"];

    for (int i = 0; i < jsonGltfBufferViews.Count(); i++) {
        GltfBufferView view;

        i32 bufIndex = jsonGltfBufferViews[i]["buffer"].ToInt();
        view.pBuffer = rawDataBuffers[bufIndex].pBytes + jsonGltfBufferViews[i]["byteOffset"].ToInt();  //@Incomplete, byte offset could not be provided, in which case we assume 0

        view.length = jsonGltfBufferViews[i]["byteLength"].ToInt();
        bufferViews.PushBack(view);
    }

    ResizableArray<GltfAccessor> accessors(pScratchArena);
    JsonValue& jsonAccessors = parsed["accessors"];
    accessors.Reserve(jsonAccessors.Count());

    for (int i = 0; i < jsonAccessors.Count(); i++) {
        GltfAccessor acc;
        JsonValue& jsonAcc = jsonAccessors[i];

        i32 idx = jsonAcc["bufferView"].ToInt();
        acc.pBuffer = bufferViews[idx].pBuffer;

        acc.count = jsonAcc["count"].ToInt();

        i32 compType = jsonAcc["componentType"].ToInt();
        switch (compType) {
            case 5120: acc.componentType = GltfAccessor::Byte; break;
            case 5121: acc.componentType = GltfAccessor::UByte; break;
            case 5122: acc.componentType = GltfAccessor::Short; break;
            case 5123: acc.componentType = GltfAccessor::UShort; break;
            case 5125: acc.componentType = GltfAccessor::UInt; break;
            case 5126: acc.componentType = GltfAccessor::f32; break;
            default: break;
        }

        String type = jsonAcc["type"].ToString();
        if (type == "SCALAR")
            acc.type = GltfAccessor::Scalar;
        else if (type == "VEC2")
            acc.type = GltfAccessor::Vec2;
        else if (type == "VEC3")
            acc.type = GltfAccessor::Vec3;
        else if (type == "VEC4")
            acc.type = GltfAccessor::Vec4;
        else if (type == "MAT2")
            acc.type = GltfAccessor::Mat2;
        else if (type == "MAT3")
            acc.type = GltfAccessor::Mat3;
        else if (type == "MAT4")
            acc.type = GltfAccessor::Mat4;

        accessors.PushBack(acc);
    }

	i32 meshCount = parsed["meshes"].Count();
	for (i32 i = 0; i < meshCount; i++) {
        JsonValue& jsonMesh = parsed["meshes"][i];
		String meshName = jsonMesh["name"].ToString();

		// make a table for the mesh
		lua_getglobal(L, "serialize");
		lua_newtable(L);

		// push mesh format (i.e. flat, smooth etc), probably just a number for now, need to do enums
		lua_pushnumber(L, 1);
		lua_setfield(L, -2, "format");

		if (jsonMesh["primitives"].Count() > 1)
		{
			Log::Warn("Mesh %s has more than one primitive, this is not supported, only the first will be imported", meshName);
		}
		JsonValue& jsonPrimitive = jsonMesh["primitives"][0];

		if (jsonPrimitive.HasKey("mode")) {
			if (jsonPrimitive["mode"].ToInt() != 4) {
				Log::Warn("Mesh %s uses unsupported topology type, will not be imported", meshName);
			}
		}

		// load, and interlace mesh data, flatten indices
		i32 nVerts = accessors[jsonPrimitive["attributes"]["POSITION"].ToInt()].count;

		JsonValue& jsonAttr = jsonPrimitive["attributes"];
		Vec3f* vertPositionBuffer = (Vec3f*)accessors[jsonAttr["POSITION"].ToInt()].pBuffer;
		Vec3f* vertNormBuffer = jsonAttr.HasKey("NORMAL") ? (Vec3f*)accessors[jsonAttr["NORMAL"].ToInt()].pBuffer : nullptr;
		Vec2f* vertTexCoordBuffer = jsonAttr.HasKey("TEXCOORD_0") ? (Vec2f*)accessors[jsonAttr["TEXCOORD_0"].ToInt()].pBuffer : nullptr;
		Vec4f* vertColBuffer = jsonAttr.HasKey("COLOR_0") ? (Vec4f*)accessors[jsonAttr["COLOR_0"].ToInt()].pBuffer : nullptr;

		// interlace vertex data
		ResizableArray<VertexData> indexedVertexData(g_pArenaFrame);
		indexedVertexData.Reserve(nVerts);
		for (int i = 0; i < nVerts; i++) {
			indexedVertexData.PushBack({
				vertPositionBuffer[i], 
				vertColBuffer ?	vertColBuffer[i] : Vec4f(1.0f, 1.0f, 1.0f, 1.0f),
				vertTexCoordBuffer ? vertTexCoordBuffer[i] : Vec2f(1.0f, 1.0f),
				vertNormBuffer ? vertNormBuffer[i] : Vec3f(1.0f, 1.0f, 1.0f)});
		}

		// Flatten indices
		i32 nIndices = accessors[jsonPrimitive["indices"].ToInt()].count;
		u16* indexBuffer = (u16*)accessors[jsonPrimitive["indices"].ToInt()].pBuffer;

		ResizableArray<VertexData> vertices(pScratchArena);
		vertices.Reserve(nIndices);
		for (int i = 0; i < nIndices; i++) {
			u16 index = indexBuffer[i];
			vertices.PushBack(indexedVertexData[index]);
		}

		// push buffer with size for all the vertex data
		i32 floatsPerVertex = sizeof(VertexData) / sizeof(f32);
		BufferLib::Buffer* pBuffer = BufferLib::AllocBuffer(L, BufferLib::Type::Float32, floatsPerVertex*nIndices, 1);
		i32 bufSize = BufferLib::GetBufferSize(pBuffer);
		memcpy((u8*)pBuffer->pData, (u8*)vertices.pData, bufSize);
		lua_setfield(L, -2, "vertices");

		// call serialize to make a file for the mesh
		lua_pushnumber(L, format);
		lua_pcall(L, 2, 1, 0);

		String result;
		size_t len;
		result.pData = (char*)lua_tolstring(L, -1, &len); 
		result.length = (i64)len;

		StringBuilder builder(pScratchArena);
		builder.Append(outputPath);
		builder.Append(meshName);
		builder.Append(".mesh");
		SDL_RWops* pOutFile = SDL_RWFromFile(builder.CreateString(pScratchArena).pData, "wb");
		SDL_RWwrite(pOutFile, result.pData, result.length, 1);
		SDL_RWclose(pOutFile);

		lua_pop(L, 1); // pop the mesh table, we're done with it now
	}

	// -------------------------------------
	// Import Textures

	// @todo

	Log::Info("Imported %s", source.pData);
	return 0;
}

};
