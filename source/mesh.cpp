// Copyright 2020-2022 David Colson. All rights reserved.

#include "Mesh.h"

#include "base64.h"
#include "defer.h"
#include "json.h"
#include "string_builder.h"
#include "image.h"

#include <SDL_rwops.h>
#include <resizable_array.inl>

// ***********************************************************************

int Primitive::GetNumVertices() {
    return (int)vertices.count;
}

// ***********************************************************************

Vec3f Primitive::GetVertexPosition(int index) {
    return vertices[index].pos;
}

// ***********************************************************************

Vec4f Primitive::GetVertexColor(int index) {
    return vertices[index].col;
}

// ***********************************************************************

Vec2f Primitive::GetVertexTexCoord(int index) {
    return vertices[index].tex;
}

// ***********************************************************************

Vec3f Primitive::GetVertexNormal(int index) {
    return vertices[index].norm;
}

// ***********************************************************************

int Primitive::GetMaterialTextureId() {
    return baseColorTexture;
}

// ***********************************************************************

Mesh::~Mesh() {
	// releases all the primitive memory as well
	ArenaFinished(pArena);
}

// ***********************************************************************

int Mesh::GetNumPrimitives() {
    return (int)primitives.count;
}

// ***********************************************************************

Primitive* Mesh::GetPrimitive(int index) {
    return &primitives[index];
}

// ***********************************************************************

// Actually owns the data
struct MeshBuffer {
    char* pBytes { nullptr };
    usize byteLength { 0 };
};

// Does not actually own the data
struct BufferView {
    // pointer to some place in a buffer
    char* pBuffer { nullptr };
    usize length { 0 };
};

struct Accessor {
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

ResizableArray<Mesh*> Mesh::LoadMeshes(Arena* pArena, const char* filePath) {
	// there's two uses of memory here, temporary working stuff, free'd inside this function
	// and stuff that's returned. The returned stuff should use the give arena, everything else should use temp
    ResizableArray<Mesh*> outMeshes(pArena);

    // Consider caching loaded json files somewhere since LoadScene and LoadMeshes are doing duplicate work here
    SDL_RWops* pFileRead = SDL_RWFromFile(filePath, "rb");
    u64 size = SDL_RWsize(pFileRead);
	char* pData = New(g_pArenaFrame, char, size);
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);

    String file;
    file.pData = pData;
    file.length = size;
	
    JsonValue parsed = ParseJsonFile(g_pArenaFrame, file);

    bool validGltf = parsed["asset"]["version"].ToString() == "2.0";
    if (!validGltf)
        return ResizableArray<Mesh*>();

    ResizableArray<MeshBuffer> rawDataBuffers(g_pArenaFrame);
    JsonValue& jsonBuffers = parsed["buffers"];
    for (int i = 0; i < jsonBuffers.Count(); i++) {
        MeshBuffer buf;
        buf.byteLength = jsonBuffers[i]["byteLength"].ToInt();

        String encodedBuffer = jsonBuffers[i]["uri"].ToString();
		// todo: the substr 37 tells us which type of gltf file this is, glb, bin as separate file, or base 64
		// these should be the options:
		// data:dontcaretext;base64,
			// this of course is what we have here, a base64 encoded string, embedded in the uri
		// filename.bin
			// this will be another file to load, with the binary data directly there
		// no entry other than byte length, then it is glb and the binary is in the next chunk or chunks
        String decoded = DecodeBase64(g_pArenaFrame, encodedBuffer.SubStr(37));
        buf.pBytes = decoded.pData;

        rawDataBuffers.PushBack(buf);
    }

    ResizableArray<BufferView> bufferViews(g_pArenaFrame);
    JsonValue& jsonBufferViews = parsed["bufferViews"];

    for (int i = 0; i < jsonBufferViews.Count(); i++) {
        BufferView view;

        i32 bufIndex = jsonBufferViews[i]["buffer"].ToInt();
        view.pBuffer = rawDataBuffers[bufIndex].pBytes + jsonBufferViews[i]["byteOffset"].ToInt();  //@Incomplete, byte offset could not be provided, in which case we assume 0

        view.length = jsonBufferViews[i]["byteLength"].ToInt();
        bufferViews.PushBack(view);
    }

    ResizableArray<Accessor> accessors(g_pArenaFrame);
    JsonValue& jsonAccessors = parsed["accessors"];
    accessors.Reserve(jsonAccessors.Count());

    for (int i = 0; i < jsonAccessors.Count(); i++) {
        Accessor acc;
        JsonValue& jsonAcc = jsonAccessors[i];

        i32 idx = jsonAcc["bufferView"].ToInt();
        acc.pBuffer = bufferViews[idx].pBuffer;

        acc.count = jsonAcc["count"].ToInt();

        i32 compType = jsonAcc["componentType"].ToInt();
        switch (compType) {
            case 5120: acc.componentType = Accessor::Byte; break;
            case 5121: acc.componentType = Accessor::UByte; break;
            case 5122: acc.componentType = Accessor::Short; break;
            case 5123: acc.componentType = Accessor::UShort; break;
            case 5125: acc.componentType = Accessor::UInt; break;
            case 5126: acc.componentType = Accessor::f32; break;
            default: break;
        }

        String type = jsonAcc["type"].ToString();
        if (type == "SCALAR")
            acc.type = Accessor::Scalar;
        else if (type == "VEC2")
            acc.type = Accessor::Vec2;
        else if (type == "VEC3")
            acc.type = Accessor::Vec3;
        else if (type == "VEC4")
            acc.type = Accessor::Vec4;
        else if (type == "MAT2")
            acc.type = Accessor::Mat2;
        else if (type == "MAT3")
            acc.type = Accessor::Mat3;
        else if (type == "MAT4")
            acc.type = Accessor::Mat4;

        accessors.PushBack(acc);
    }

    outMeshes.Reserve(parsed["meshes"].Count());
    for (int i = 0; i < parsed["meshes"].Count(); i++) {
        JsonValue& jsonMesh = parsed["meshes"][i];

		Arena* pMeshArena = ArenaCreate();
        Mesh* pMesh = New(pMeshArena, Mesh);
		PlacementNew(pMesh) Mesh();
		pMesh->Retain();
		pMesh->pArena = pMeshArena;
		pMesh->primitives.pArena = pMesh->pArena;
        pMesh->primitives.Reserve(parsed["primitives"].Count());

        String meshName = jsonMesh.HasKey("name") ? jsonMesh["name"].ToString() : String("");
        pMesh->name = CopyString(meshName, pMesh->pArena);

        for (int j = 0; j < jsonMesh["primitives"].Count(); j++) {
            JsonValue& jsonPrimitive = jsonMesh["primitives"][j];
            Primitive prim;
			prim.vertices.pArena = pMesh->pArena;

            if (jsonPrimitive.HasKey("mode")) {
                if (jsonPrimitive["mode"].ToInt() != 4) {
                    return ResizableArray<Mesh*>();  // Unsupported topology type
                }
            }

            // Get material texture
            if (jsonPrimitive.HasKey("material")) {
                i32 materialId = jsonPrimitive["material"].ToInt();
                JsonValue& jsonMaterial = parsed["materials"][materialId];
                JsonValue& pbr = jsonMaterial["pbrMetallicRoughness"];

                if (pbr.HasKey("baseColorTexture")) {
                    i32 textureId = pbr["baseColorTexture"]["index"].ToInt();
                    i32 imageId = parsed["textures"][textureId]["source"].ToInt();
                    prim.baseColorTexture = imageId;
                }
            }

            i32 nVerts = accessors[jsonPrimitive["attributes"]["POSITION"].ToInt()].count;

            JsonValue& jsonAttr = jsonPrimitive["attributes"];
            Vec3f* vertPositionBuffer = (Vec3f*)accessors[jsonAttr["POSITION"].ToInt()].pBuffer;
            Vec3f* vertNormBuffer = jsonAttr.HasKey("NORMAL") ? (Vec3f*)accessors[jsonAttr["NORMAL"].ToInt()].pBuffer : nullptr;
            Vec2f* vertTexCoordBuffer = jsonAttr.HasKey("TEXCOORD_0") ? (Vec2f*)accessors[jsonAttr["TEXCOORD_0"].ToInt()].pBuffer : nullptr;

            // interlace vertex data
            ResizableArray<VertexData> indexedVertexData(g_pArenaFrame);
            indexedVertexData.Reserve(nVerts);
			// TODO: color may be vec3 or vec4, you need to check the accessor type first
            if (jsonAttr.HasKey("COLOR_0")) {
                Vec4f* vertColBuffer = (Vec4f*)accessors[jsonAttr["COLOR_0"].ToInt()].pBuffer;
                for (int i = 0; i < nVerts; i++) {
                    indexedVertexData.PushBack({ vertPositionBuffer[i], vertColBuffer[i], vertTexCoordBuffer[i], vertNormBuffer[i] });
                }
            } else {
                for (int i = 0; i < nVerts; i++) {
                    indexedVertexData.PushBack({ vertPositionBuffer[i], Vec4f(1.0f, 1.0f, 1.0f, 1.0f), vertTexCoordBuffer[i], vertNormBuffer[i] });
                }
            }

            // Flatten indices
            i32 nIndices = accessors[jsonPrimitive["indices"].ToInt()].count;
            u16* indexBuffer = (u16*)accessors[jsonPrimitive["indices"].ToInt()].pBuffer;

            prim.vertices.Reserve(nIndices);
            for (int i = 0; i < nIndices; i++) {
                u16 index = indexBuffer[i];
                prim.vertices.PushBack(indexedVertexData[index]);
            }

            pMesh->primitives.PushBack(prim);
        }
        outMeshes.PushBack(pMesh);
    }

      return outMeshes;
}

// ***********************************************************************

ResizableArray<Image*> Mesh::LoadTextures(Arena* pArena, const char* filePath) {
	// there's two uses of memory here, temporary working stuff, free'd inside this function
	// and stuff that's returned. The returned stuff should use the give arena, everything else should use temp
    ResizableArray<Image*> outImages(pArena);

    // Consider caching loaded json files somewhere since LoadScene/LoadMeshes/LoadImages are doing duplicate work here
    SDL_RWops* pFileRead = SDL_RWFromFile(filePath, "rb");
    u64 size = SDL_RWsize(pFileRead);
	char* pData = New(g_pArenaFrame, char, size);
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);

    String file;
    file.pData = pData;
    file.length = size;

    JsonValue parsed = ParseJsonFile(g_pArenaFrame, file);

    bool validGltf = parsed["asset"]["version"].ToString() == "2.0";
    if (!validGltf)
        return ResizableArray<Image*>();

    if (parsed.HasKey("images")) {
        outImages.Reserve(parsed["images"].Count());
        for (usize i = 0; i < parsed["images"].Count(); i++) {
            JsonValue& jsonImage = parsed["images"][i];
            String type = jsonImage["mimeType"].ToString();

			// TODO: if the image has a bufferView property, then it's in the buffer
			// either a nearby bin, or base64, so load it from there explicitely, rather than finding a file
			// only load a file if there is a uri, which you can interpret as a file path, relative to the parent file

            StringBuilder builder(g_pArenaFrame);
			// todo: replace when we have VFS
            builder.Append("systemroot/tank_demo/");
            builder.Append(jsonImage["name"].ToString());
            builder.Append(".");
            builder.Append(type.SubStr(6, 4));

            String imagePath = builder.CreateString(g_pArenaFrame);
			// bad, don't use new, fix this
            Image* pImage = new Image(imagePath);
            outImages.PushBack(pImage);
        }
    }
    return outImages;
}
