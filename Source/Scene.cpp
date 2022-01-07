#include "Scene.h"

#include "Core/Json.h"
#include "Core/Base64.h"

#include <SDL_rwops.h>
#include <string>

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

Scene* LoadScene(const char* filePath)
{
    SDL_RWops* pFileRead = SDL_RWFromFile(filePath, "rb");

    Scene* pScene = new Scene();

    uint64_t size = SDL_RWsize(pFileRead);
    char* pData = new char[size];
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);
    std::string file(pData, pData + size);
    delete[] pData;

    JsonValue parsed = ParseJsonFile(file);

    bool validGltf = parsed["asset"]["version"].ToString() == "2.0";
    if (!validGltf)
        return nullptr;

    pScene->m_nodes.reserve(parsed["nodes"].Count());
    ParseNodesRecursively(nullptr, pScene->m_nodes, parsed["scenes"][0]["nodes"], parsed["nodes"]);

    return pScene;
}