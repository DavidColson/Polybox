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
		UserData* pUserData = AllocUserData(L, Type::Float32, 3, 1);
		f32* pFloats = (f32*)pUserData->pData;
		pFloats[0] = (f32)nodeToParse["translation"][0].ToFloat();
		pFloats[1] = (f32)nodeToParse["translation"][1].ToFloat();
		pFloats[2] = (f32)nodeToParse["translation"][2].ToFloat();
	}
	else {
		UserData* pUserData = AllocUserData(L, Type::Float32, 3, 1);
	}
	lua_setfield(L, -2, "position");

	// rotation
	if (nodeToParse.HasKey("rotation")) {
		UserData* pUserData = AllocUserData(L, Type::Float32, 4, 1);
		f32* pFloats = (f32*)pUserData->pData;
		pFloats[0] = (f32)nodeToParse["rotation"][0].ToFloat();
		pFloats[1] = (f32)nodeToParse["rotation"][1].ToFloat();
		pFloats[2] = (f32)nodeToParse["rotation"][2].ToFloat();
		pFloats[3] = (f32)nodeToParse["rotation"][3].ToFloat();
	}
	else {
		UserData* pUserData = AllocUserData(L, Type::Float32, 4, 1);
	}
	lua_setfield(L, -2, "rotation");

	// scale
	if (nodeToParse.HasKey("scale")) {
		UserData* pUserData = AllocUserData(L, Type::Float32, 3, 1);
		f32* pFloats = (f32*)pUserData->pData;
		pFloats[0] = (f32)nodeToParse["scale"][0].ToFloat();
		pFloats[1] = (f32)nodeToParse["scale"][1].ToFloat();
		pFloats[2] = (f32)nodeToParse["scale"][2].ToFloat();
	}
	else {
		UserData* pUserData = AllocUserData(L, Type::Float32, 3, 1);
		f32* pFloats = (f32*)pUserData->pData;
		pFloats[0] = 1.0f;
		pFloats[1] = 1.0f;
		pFloats[2] = 1.0f;
	}
	lua_setfield(L, -2, "scale");

	// mesh identifier
	if (nodeToParse.HasKey("mesh")) {
		i32 meshId = nodeToParse["mesh"].ToInt();
		
		if (gltf["meshes"][meshId].HasKey("name")) {
			String meshName = gltf["meshes"][meshId]["name"].ToString();
			lua_pushlstring(L, meshName.pData, meshName.length);
		}
		else {
			String meshName = TempPrint("mesh%d", meshId);
			lua_pushlstring(L, meshName.pData, meshName.length);
		}
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
    u8* pBytes { nullptr };
    u64 byteLength { 0 };
};

// Does not actually own the data
struct GltfBufferView {
    // pointer to some place in a buffer
    u8* pBuffer { nullptr };
    u64 length { 0 };
};

struct GltfAccessor {
    // pointer to some place in a buffer view
    u8* pBuffer { nullptr };
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

struct ImportTableAsset {
	String sourceAsset;
	bool enableAutoImport;
	i64 lastImportTime;
};

struct AssetImportTable {
	Arena* pArena;
	HashMap<String, ImportTableAsset> table;
};

// ***********************************************************************

AssetImportTable* LoadImportTable(String project) {
	// create a new one if this project has no import table
	String importTablePath = TempPrint("system/%S/import_table.txt", project);
	// if (!FileExists(importTablePath)) {
	{
		Arena* pArena = ArenaCreate();
		AssetImportTable* pImportTable = New(pArena, AssetImportTable);
		pImportTable->pArena = pArena;
		pImportTable->table.pArena = pArena;
		return pImportTable;
	}
	// @todo: actually load and parse a text file into the import table
	return nullptr;
}

// ***********************************************************************

bool SaveImportTable(AssetImportTable* pTable, String project) {
	StringBuilder builder(g_pArenaFrame);

	for (i64 i = 0; i < pTable->table.tableSize; i++) {
		HashNode<String, ImportTableAsset>& node = pTable->table.pTable[i];
		if (node.hash != UNUSED_HASH) {
			builder.AppendFormat("%S %S %s %i\n", node.key, node.value.sourceAsset, node.value.enableAutoImport ? "1" : "0", node.value.lastImportTime);
		}
	}
	String importTablePath = TempPrint("system/%S/import_table.txt", project);
	String fileContent = builder.CreateString(g_pArenaFrame);
	WriteWholeFile(importTablePath, fileContent.pData, fileContent.length);
	return true;
}

// ***********************************************************************

int Import(Arena* pScratchArena, u8 format, String source, String output) {
	// TODO: take the file extension as an indication of what the file is
	// below we are doing gltf only. 

	// if first few bytes is "gltf" in ascii, then it's a binary, glb file, you can verify this with the extension

	// note that if we import a whole folder, we may accidentally import files referenced by the
	// gltf, so when importing seperate gltf files, don't import stuff we already may have picked up, such as images
	// they'll be done individually

	Log::Info("Importing %S", source);

	String projectName;
	for (i64 i = 0; i < output.length; i++) {
		if (output.pData[i] == '\\' || output.pData[i] == '/') {
			projectName = SubStr(output, 0, i);
			break;
		}
	}

	// @todo: temporary, this assumes that the output is actually a file, 
	// it could be a path itself in which case we must work out the filename instead
	// A better approach for all of this would be to just write a function that splits a string into 
	// sections on a delimeter, we don't need strtok because we can just make string views
	// then the first element is the project name
	// We check if the last char is a "/" or "\\", if so we treat it as a path, otherwise we treat it as a file
	// Then you can build your output folders from the pieces
	String outputFolder;
	String outputFilename;
	for (i64 i = output.length-1; i>=0; i--) {
		if (output.pData[i] == '\\' || output.pData[i] == '/') {
			outputFolder = SubStr(output, 0, i+1);
			outputFilename = SubStr(output, i+1, output.length);
			break;
		}
	}
	// todo: normalize path separators here, we end up with a mix depending on what came into the function
	String outputPath = TempPrint("system/%S", outputFolder);

	AssetImportTable* pImportTable = LoadImportTable(projectName);

	// load the asset table for the target project
	// you will want to de-duplicate, since you may be re-importing stuff that's already in the table
	// What does the table look like in memory? A hash table?

	// Load file
	String fileContents;
    fileContents.pData = ReadWholeFile(source.pData, &fileContents.length, pScratchArena);

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
	BindSerialization(L);
	BindUserData(L);

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

		String outputFilepath = StringPrint(pScratchArena, "%S%S", outputPath, outputFilename);
		if (WriteWholeFile(outputFilepath, result.pData, result.length)) {
			Log::Info("	Exported %S", outputFilepath);
			pImportTable->table[outputFilepath] = ImportTableAsset{ .sourceAsset = source, .enableAutoImport = true, .lastImportTime = 978912};
		}
		else {
			Log::Info("	Failed to export %S, is the path correct?", outputFilepath);
			return -1;
		}
	}
	lua_pop(L, 1); // pop the scene table, we're done with it now

	// -------------------------------------
	// Load buffer data

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
        String decoded = DecodeBase64(pScratchArena, SubStr(encodedBuffer, 37));
        buf.pBytes = (u8*)decoded.pData;

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

	// -------------------------------------
	// Import Meshes

	i32 meshCount = parsed["meshes"].Count();
	for (i32 i = 0; i < meshCount; i++) {
        JsonValue& jsonMesh = parsed["meshes"][i];

		String meshName;
		if (jsonMesh.HasKey("name")) {
			meshName = jsonMesh["name"].ToString();
		}
		else {
			meshName = TempPrint("mesh%d", i);
		}

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

		// push texture
		if (jsonPrimitive.HasKey("material")) {
			i32 materialId = jsonPrimitive["material"].ToInt();
			JsonValue& jsonMaterial = parsed["materials"][materialId];
			JsonValue& pbr = jsonMaterial["pbrMetallicRoughness"];

			if (pbr.HasKey("baseColorTexture")) {
				i32 textureId = pbr["baseColorTexture"]["index"].ToInt();
				i32 imageId = parsed["textures"][textureId]["source"].ToInt();
				String imageName;
				if (parsed["images"][imageId].HasKey("name")) {
					imageName = parsed["images"][imageId]["name"].ToString(); 
				}
				else {
					imageName = TempPrint("image%d", imageId);
				}
				lua_pushlstring(L, imageName.pData, imageName.length);
				lua_setfield(L, -2, "texture");
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
		UserData* pUserData = AllocUserData(L, Type::Float32, floatsPerVertex*nIndices, 1);
		i64 bufSize = GetUserDataSize(pUserData);
		memcpy((u8*)pUserData->pData, (u8*)vertices.pData, bufSize);
		lua_setfield(L, -2, "vertices");

		// call serialize to make a file for the mesh
		lua_pushnumber(L, format);
		lua_pcall(L, 2, 1, 0);

		String result;
		size_t len;
		result.pData = (char*)lua_tolstring(L, -1, &len); 
		result.length = (i64)len;

		String outputFilepath = StringPrint(pScratchArena, "%S%S.mesh", outputPath, meshName);
		if (WriteWholeFile(outputFilepath, result.pData, result.length)) {
			Log::Info("	Exported %S", outputFilepath);
			pImportTable->table[outputFilepath] = ImportTableAsset{ .sourceAsset = source, .enableAutoImport = true, .lastImportTime = 978912};
		}
		else {
			Log::Info("	Failed to export %S, is the path correct?", outputFilepath);
			return -1;
		}

		lua_pop(L, 1); // pop the mesh table, we're done with it now
	}

	// -------------------------------------
	// Import Textures

	i32 imageCount = parsed["images"].Count();
	for (i32 i = 0; i < imageCount; i++) {
		JsonValue& jsonImage = parsed["images"][i];
		String imageName;
		if (jsonImage.HasKey("name")) {
			imageName = jsonImage["name"].ToString(); 
		}
		else {
			imageName = TempPrint("image%d", i);
		}

		u8* pImageRawData = nullptr;
		i32 imageDataLen = 0;

		if (jsonImage.HasKey("bufferView")) {
			i32 bufferId = jsonImage["bufferView"].ToInt(); 
			pImageRawData = bufferViews[bufferId].pBuffer;
			imageDataLen = (i32)bufferViews[bufferId].length;
		}
		else if (jsonImage.HasKey("uri")) {
			// two possibilities, the binary data is embedded in the uri string
			// or it's a filename

			String uri = jsonImage["uri"].ToString();
			if (Find(uri, "data:image/png;base64,") == 0) {
				String decoded = DecodeBase64(pScratchArena, SubStr(uri, 22));
				pImageRawData = (u8*)decoded.pData;
				imageDataLen = (i32)decoded.length;
			}
			else {
				Log::Warn("Unable to import image %s, we don't yet support external image uris", jsonImage["name"].ToString().pData);
				return -1;
			}
		}
		else {
			Log::Warn("Unable to import image %s, we can't find the data in the gltf", jsonImage["name"].ToString().pData);
			return -1;
		}

		lua_getglobal(L, "serialize");
		lua_newtable(L);

		i32 n;
		i32 width;
		i32 height;
		u8* pData = stbi_load_from_memory((const unsigned char*)pImageRawData, imageDataLen, &width, &height, &n, 4);
		if (pData == nullptr) {
			Log::Warn("Failed to load image %s: %s", imageName.pData, stbi_failure_reason());
			continue;
		}
		
		// upload image to lua
		lua_pushnumber(L, width);
		lua_setfield(L, -2, "width");
		lua_pushnumber(L, height);
		lua_setfield(L, -2, "height");

		UserData* pUserData = AllocUserData(L, Type::Int32, width, height);
		i64 bufSize = GetUserDataSize(pUserData);
		memcpy((u8*)pUserData->pData, pData, bufSize);
		lua_setfield(L, -2, "data");

		// call serialize to make a file for the image
		lua_pushnumber(L, format);
		lua_pcall(L, 2, 1, 0);

		String result;
		size_t len;
		result.pData = (char*)lua_tolstring(L, -1, &len); 
		result.length = (i64)len;

		String outputFilepath = StringPrint(pScratchArena, "%S%S.texture", outputPath, imageName);
		if (WriteWholeFile(outputFilepath, result.pData, result.length)) {
			Log::Info("	Exported %S", outputFilepath);
			pImportTable->table[outputFilepath] = ImportTableAsset{ .sourceAsset = source, .enableAutoImport = true, .lastImportTime = 978912};
		}
		else {
			Log::Info("	Failed to export %S, is the path correct?", outputFilepath);
			return -1;
		}

		stbi_image_free(pData);
		lua_pop(L, 1);
	}

	SaveImportTable(pImportTable, projectName);

	return 0;
}

};
