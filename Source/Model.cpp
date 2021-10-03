#include "Model.h"

#include "Core/Json.h"
#include "Core/Base64.h"

#include <SDL_rwops.h>

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

void ParseNodesRecursively(Node* pParent, std::vector<Node>& outNodes, JsonValue& nodeToParse, JsonValue& nodesData)
{
    for (int i = 0; i < nodeToParse.Count(); i++)
    {
        int nodeId = nodeToParse[i].ToInt();
        JsonValue& jsonNode = nodesData[nodeId];

        // extract the nodes
        outNodes.emplace_back();
        Node& node = outNodes.back();

        node.m_name = jsonNode.HasKey("name") ? jsonNode["name"].ToString() : "";
        node.m_meshId = UINT32_MAX;

        if (jsonNode.HasKey("children"))
        {
            ParseNodesRecursively(&node, outNodes, jsonNode["children"], nodesData);
        }

        if (pParent)
        {
            pParent->m_children.push_back(&node);
            node.m_pParent = pParent;
        }

        if (jsonNode.HasKey("mesh"))
        {
            node.m_meshId = jsonNode["mesh"].ToInt();
        }

        if (jsonNode.HasKey("rotation"))
        {
            node.m_rotation.x = float(jsonNode["rotation"][0].ToFloat());
            node.m_rotation.y = float(jsonNode["rotation"][1].ToFloat());
            node.m_rotation.z = float(jsonNode["rotation"][2].ToFloat());
            node.m_rotation.w = float(jsonNode["rotation"][3].ToFloat());
        }
        else
        {
            node.m_rotation = Quatf::Identity();
        }

        if (jsonNode.HasKey("translation"))
        {
            node.m_translation.x = float(jsonNode["translation"][0].ToFloat());
            node.m_translation.y = float(jsonNode["translation"][1].ToFloat());
            node.m_translation.z = float(jsonNode["translation"][2].ToFloat());
        }
        else
        {
            node.m_translation = Vec3f(0.0f);
        }

        if (jsonNode.HasKey("scale"))
        {
            node.m_scale.x = float(jsonNode["scale"][0].ToFloat());
            node.m_scale.y = float(jsonNode["scale"][1].ToFloat());
            node.m_scale.z = float(jsonNode["scale"][2].ToFloat());
        }
        else
        {
            node.m_scale = Vec3f(1.0f);
        }
    } 
}

Scene::Scene(std::string filePath)
{
    SDL_RWops* pFileRead = SDL_RWFromFile(filePath.c_str(), "rb");

    uint64_t size = SDL_RWsize(pFileRead);
    char* pData = new char[size];
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);

    std::string file(pData, pData + size);

    JsonValue parsed = ParseJsonFile(file);

    bool validGltf = parsed["asset"]["version"].ToString() == "2.0";
    if (!validGltf)
        return;

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
    
    m_nodes.reserve(parsed["nodes"].Count());
    ParseNodesRecursively(nullptr, m_nodes, parsed["scenes"][0]["nodes"], parsed["nodes"]);

    if (parsed.HasKey("images"))
    {
        m_images.reserve(parsed["images"].Count());
        for (size_t i = 0; i < parsed["images"].Count(); i++)
        {
            JsonValue& jsonImage = parsed["images"][i];
            std::string type = jsonImage["mimeType"].ToString();
            std::string imagePath = "Assets/" + jsonImage["name"].ToString() + "." + type.substr(6, 4);
            m_images.emplace_back(imagePath);
        }
    }

    m_meshes.reserve(parsed["meshes"].Count());
    for (int i = 0; i < parsed["meshes"].Count(); i++)
    {
        JsonValue& jsonMesh = parsed["meshes"][i];

        Mesh mesh;
        mesh.m_name = jsonMesh.HasKey("name") ? jsonMesh["name"].ToString() : "";

        for (int j = 0; j < jsonMesh["primitives"].Count(); j++)
        {
            JsonValue& jsonPrimitive = jsonMesh["primitives"][j];
            Primitive prim;

            if (jsonPrimitive.HasKey("mode"))
            {
                if (jsonPrimitive["mode"].ToInt() != 4)
                {
                    return; // Unsupported topology type
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
                if (pbr.HasKey("baseColorFactor"))
                {
                    prim.m_baseColor.x = (float)pbr["baseColorFactor"][0].ToFloat();
                    prim.m_baseColor.y = (float)pbr["baseColorFactor"][1].ToFloat();
                    prim.m_baseColor.z = (float)pbr["baseColorFactor"][2].ToFloat();
                    prim.m_baseColor.w = (float)pbr["baseColorFactor"][3].ToFloat();
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

            mesh.m_primitives.push_back(std::move(prim));
        }
        m_meshes.push_back(std::move(mesh));
    }
    
    for (int i = 0; i < rawDataBuffers.size(); i++)
    {
        delete rawDataBuffers[i].pBytes;
    }
}