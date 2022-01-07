#include "Mesh.h"

#include "Core/Json.h"
#include "Core/Base64.h"

#include <SDL_rwops.h>
#include <string>


// ***********************************************************************

int Primitive::GetNumVertices()
{
    return (int)m_vertices.size();
}

// ***********************************************************************

Vec3f Primitive::GetVertexPosition(int index)
{
    return m_vertices[index].pos;
}

// ***********************************************************************

Vec4f Primitive::GetVertexColor(int index)
{
    return m_vertices[index].col;
}

// ***********************************************************************

Vec2f Primitive::GetVertexTexCoord(int index)
{
    return m_vertices[index].tex;
}

// ***********************************************************************

Vec3f Primitive::GetVertexNormal(int index)
{
    return m_vertices[index].norm;
}

// ***********************************************************************

int Primitive::GetMaterialTextureId()
{
    return m_baseColorTexture;
}

// ***********************************************************************

Mesh::~Mesh()
{
}

// ***********************************************************************

int Mesh::GetNumPrimitives()
{
    return (int)m_primitives.size();
}

// ***********************************************************************

Primitive* Mesh::GetPrimitive(int index)
{
    return &m_primitives[index];
}

// ***********************************************************************

// Actually owns the data
struct Buffer
{
    char* pBytes{ nullptr };
    size_t byteLength{ 0 };
};

// Does not actually own the data
struct BufferView
{
    // pointer to some place in a buffer
    char* pBuffer{ nullptr };
    size_t length{ 0 };

    enum Target
    {
        Array,
        ElementArray
    };
    Target target;    
};

struct Accessor
{
    // pointer to some place in a buffer view
    char* pBuffer{ nullptr };
    int count{ 0 };
    enum ComponentType
    {
        Byte,
        UByte,
        Short,
        UShort,
        UInt,
        Float
    };
    ComponentType componentType;

    enum Type
    {
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

std::vector<Mesh*> Mesh::LoadMeshes(const char* filePath)
{
    std::vector<Mesh*> outMeshes;

    // Consider caching loaded json files somewhere since LoadScene and LoadMeshes are doing duplicate work here
    SDL_RWops* pFileRead = SDL_RWFromFile(filePath, "rb");
    uint64_t size = SDL_RWsize(pFileRead);
    char* pData = new char[size];
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);
    std::string file(pData, pData + size);
    delete[] pData;

    JsonValue parsed = ParseJsonFile(file);

    bool validGltf = parsed["asset"]["version"].ToString() == "2.0";
    if (!validGltf)
        return std::vector<Mesh*>();

    std::vector<Buffer> rawDataBuffers;
    JsonValue& jsonBuffers = parsed["buffers"];
    for (int i = 0; i < jsonBuffers.Count(); i++)
    {
        Buffer buf;
        buf.byteLength = jsonBuffers[i]["byteLength"].ToInt();
        buf.pBytes = new char[buf.byteLength];
        
        std::string encodedBuffer = jsonBuffers[i]["uri"].ToString().substr(37);
        memcpy(buf.pBytes, DecodeBase64(encodedBuffer).data(), buf.byteLength);

        rawDataBuffers.push_back(buf);
    }

    std::vector<BufferView> bufferViews;
    JsonValue& jsonBufferViews = parsed["bufferViews"];

    for (int i = 0; i < jsonBufferViews.Count(); i++)
    {
        BufferView view;

        int bufIndex = jsonBufferViews[i]["buffer"].ToInt();
        view.pBuffer = rawDataBuffers[bufIndex].pBytes + jsonBufferViews[i]["byteOffset"].ToInt(); //@Incomplete, byte offset could not be provided, in which case we assume 0

        view.length = jsonBufferViews[i]["byteLength"].ToInt();

        // @Incomplete, target may not be provided
        int target = jsonBufferViews[i]["target"].ToInt();
        if (target == 34963)
            view.target = BufferView::ElementArray;
        else if (target = 34962)
            view.target = BufferView::Array;
        bufferViews.push_back(view);
    }

    std::vector<Accessor> accessors;
    JsonValue& jsonAccessors = parsed["accessors"];
    accessors.reserve(jsonAccessors.Count());

    for (int i = 0; i < jsonAccessors.Count(); i++)
    {
        Accessor acc;
        JsonValue& jsonAcc = jsonAccessors[i];

        int idx = jsonAcc["bufferView"].ToInt();
        acc.pBuffer = bufferViews[idx].pBuffer + jsonAcc["byteOffset"].ToInt();
        
        acc.count = jsonAcc["count"].ToInt();

        int compType = jsonAcc["componentType"].ToInt();
        switch (compType)
        {
        case 5120: acc.componentType = Accessor::Byte; break;
        case 5121: acc.componentType = Accessor::UByte; break;
        case 5122: acc.componentType = Accessor::Short; break;
        case 5123: acc.componentType = Accessor::UShort; break;
        case 5125: acc.componentType = Accessor::UInt; break;
        case 5126: acc.componentType = Accessor::Float; break;
        default: break;
        }

        std::string type = jsonAcc["type"].ToString();
        if (type == "SCALAR") acc.type = Accessor::Scalar;
        else if (type == "VEC2") acc.type = Accessor::Vec2;
        else if (type == "VEC3") acc.type = Accessor::Vec3;
        else if (type == "VEC4") acc.type = Accessor::Vec4;
        else if (type == "MAT2") acc.type = Accessor::Mat2;
        else if (type == "MAT3") acc.type = Accessor::Mat3;
        else if (type == "MAT4") acc.type = Accessor::Mat4;

        accessors.push_back(acc);
    }
    
    outMeshes.reserve(parsed["meshes"].Count());
    for (int i = 0; i < parsed["meshes"].Count(); i++)
    {
        JsonValue& jsonMesh = parsed["meshes"][i];

        Mesh* pMesh = new Mesh();
        pMesh->m_name = jsonMesh.HasKey("name") ? jsonMesh["name"].ToString() : "";

        for (int j = 0; j < jsonMesh["primitives"].Count(); j++)
        {
            JsonValue& jsonPrimitive = jsonMesh["primitives"][j];
            Primitive prim;

            if (jsonPrimitive.HasKey("mode"))
            {
                if (jsonPrimitive["mode"].ToInt() != 4)
                {
                    return std::vector<Mesh*>(); // Unsupported topology type
                }
            }

            // Get material texture
            if (jsonPrimitive.HasKey("material"))
            {
                int materialId = jsonPrimitive["material"].ToInt();
                JsonValue& jsonMaterial = parsed["materials"][materialId];
                JsonValue& pbr = jsonMaterial["pbrMetallicRoughness"];

                if (pbr.HasKey("baseColorTexture"))
                {
                    int textureId = pbr["baseColorTexture"]["index"].ToInt();
                    int imageId = parsed["textures"][textureId]["source"].ToInt();
                    prim.m_baseColorTexture = imageId;
                }
            }

            int nVerts = accessors[jsonPrimitive["attributes"]["POSITION"].ToInt()].count;

            JsonValue& jsonAttr = jsonPrimitive["attributes"];
            Vec3f* vertPositionBuffer = (Vec3f*)accessors[jsonAttr["POSITION"].ToInt()].pBuffer;
            Vec3f* vertNormBuffer = jsonAttr.HasKey("NORMAL") ? (Vec3f*)accessors[jsonAttr["NORMAL"].ToInt()].pBuffer : nullptr;
            Vec2f* vertTexCoordBuffer = jsonAttr.HasKey("TEXCOORD_0") ? (Vec2f*)accessors[jsonAttr["TEXCOORD_0"].ToInt()].pBuffer : nullptr;

            // Interlace vertex data
            std::vector<VertexData> indexedVertexData;
            indexedVertexData.reserve(nVerts);
            if (jsonAttr.HasKey("COLOR_0"))
            {
                Vec4f* vertColBuffer = (Vec4f*)accessors[jsonAttr["COLOR_0"].ToInt()].pBuffer;
                for (int i = 0; i < nVerts; i++)
                {
                    indexedVertexData.push_back({vertPositionBuffer[i], vertColBuffer[i], vertTexCoordBuffer[i], vertNormBuffer[i]});
                }
            }
            else
            {
                for (int i = 0; i < nVerts; i++)
                {
                    indexedVertexData.push_back({vertPositionBuffer[i], Vec4f(1.0f, 1.0f, 1.0f, 1.0f), vertTexCoordBuffer[i], vertNormBuffer[i]});
                }
            }
            
            // Flatten indices
            int nIndices = accessors[jsonPrimitive["indices"].ToInt()].count;
            uint16_t* indexBuffer = (uint16_t*)accessors[jsonPrimitive["indices"].ToInt()].pBuffer;

            prim.m_vertices.reserve(nIndices);
            for (int i = 0; i < nIndices; i++)
            {
                uint16_t index = indexBuffer[i];
                prim.m_vertices.push_back(indexedVertexData[index]);
            }

            pMesh->m_primitives.push_back(std::move(prim));
        }
        outMeshes.push_back(pMesh);
    }

    for (int i = 0; i < rawDataBuffers.size(); i++)
    {
        delete rawDataBuffers[i].pBytes;
    }
    return std::move(outMeshes);
}

// ***********************************************************************

std::vector<Image*> Mesh::LoadTextures(const char* filePath)
{
    std::vector<Image*> outImages;

    // Consider caching loaded json files somewhere since LoadScene/LoadMeshes/LoadImages are doing duplicate work here
    SDL_RWops* pFileRead = SDL_RWFromFile(filePath, "rb");
    uint64_t size = SDL_RWsize(pFileRead);
    char* pData = new char[size];
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);
    std::string file(pData, pData + size);
    delete[] pData;

    JsonValue parsed = ParseJsonFile(file);

    bool validGltf = parsed["asset"]["version"].ToString() == "2.0";
    if (!validGltf)
        return std::vector<Image*>();

    if (parsed.HasKey("images"))
    {
        outImages.reserve(parsed["images"].Count());
        for (size_t i = 0; i < parsed["images"].Count(); i++)
        {
            JsonValue& jsonImage = parsed["images"][i];
            std::string type = jsonImage["mimeType"].ToString();
            std::string imagePath = "Assets/" + jsonImage["name"].ToString() + "." + type.substr(6, 4);
            Image* pImage = new Image(imagePath);
            outImages.emplace_back(pImage);
        }
    }
    return std::move(outImages);
}