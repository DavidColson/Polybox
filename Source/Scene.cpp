// Copyright 2020-2022 David Colson. All rights reserved.

#include "Scene.h"

#include "Core/Json.h"
#include "Core/Base64.h"

#include <SDL_rwops.h>
#include <string>

uint64_t Node::s_nodeIdCounter = 0;

// ***********************************************************************

Vec3f Node::GetLocalPosition()
{
    return m_localTransform.GetTranslation();
}

// ***********************************************************************

Vec3f Node::GetWorldPosition()
{
    return m_worldTransform.GetTranslation();
}

// ***********************************************************************

void Node::SetLocalPosition(Vec3f translation)
{
    m_localTransform.SetTranslation(translation);
    UpdateWorldTransforms();
}

// ***********************************************************************

Vec3f Node::GetLocalRotation()
{
    return m_localTransform.GetEulerRotation();
}

// ***********************************************************************

Vec3f Node::GetWorldRotation()
{
    return m_worldTransform.GetEulerRotation();
}

// ***********************************************************************

void Node::SetLocalRotation(Vec3f rotation)
{
    m_localTransform.SetEulerRotation(rotation);
    UpdateWorldTransforms();
}

// ***********************************************************************

void Node::SetLocalRotation(Quatf rotation)
{
    m_localTransform.SetQuatRotation(rotation);
    UpdateWorldTransforms();
}

// ***********************************************************************

Vec3f Node::GetLocalScale()
{
    return m_localTransform.GetScaling();
}

// ***********************************************************************

Vec3f Node::GetWorldScale()
{
    return m_worldTransform.GetScaling();
}

// ***********************************************************************

void Node::SetLocalScale(Vec3f scale)
{
    m_localTransform.SetScaling(scale);
    UpdateWorldTransforms();
}

// ***********************************************************************

Node* Node::GetParent()
{
    return m_pParent;
}

// ***********************************************************************

int Node::GetNumChildren()
{
    return (int)m_children.size();
}

// ***********************************************************************

Node* Node::GetChild(int index)
{
    return m_children[index];
}

// ***********************************************************************

void Node::UpdateWorldTransforms()
{
    // Recalculate local world matrix
    if (m_pParent)
    {
        m_worldTransform = m_pParent->m_worldTransform * m_localTransform;
    }
    else
    {
        m_worldTransform = m_localTransform;
    }

    if (!m_children.empty())
    {
        for (Node* pChild : m_children)
        {
            pChild->m_worldTransform = m_worldTransform * pChild->m_localTransform;
        }
    }
}

// ***********************************************************************

int Scene::GetNumNodes()
{
    return (int)m_nodes.size();
}

// ***********************************************************************

Node* Scene::GetNode(int index)
{
    return &m_nodes[index];
}

// ***********************************************************************

void ParseNodesRecursively(Scene* pScene, Node* pParent, std::vector<Node>& outNodes, JsonValue& nodeToParse, JsonValue& nodesData)
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
            Quatf rotation;
            rotation.x = float(jsonNode["rotation"][0].ToFloat());
            rotation.y = float(jsonNode["rotation"][1].ToFloat());
            rotation.z = float(jsonNode["rotation"][2].ToFloat());
            rotation.w = float(jsonNode["rotation"][3].ToFloat());
            node.SetLocalRotation(rotation);
        }
        else
        {
            node.SetLocalRotation(Quatf::Identity());
        }

        if (jsonNode.HasKey("translation"))
        {
            Vec3f translation;
            translation.x = float(jsonNode["translation"][0].ToFloat());
            translation.y = float(jsonNode["translation"][1].ToFloat());
            translation.z = float(jsonNode["translation"][2].ToFloat());
            node.SetLocalPosition(translation);
        }
        else
        {
            node.SetLocalPosition(Vec3f(0.0f));
        }

        if (jsonNode.HasKey("scale"))
        {
            Vec3f scale;
            scale.x = float(jsonNode["scale"][0].ToFloat());
            scale.y = float(jsonNode["scale"][1].ToFloat());
            scale.z = float(jsonNode["scale"][2].ToFloat());
            node.SetLocalScale(scale);
        }
        else
        {
            node.SetLocalScale(Vec3f(1.0f));
        }

        if (jsonNode.HasKey("children"))
        {
            ParseNodesRecursively(pScene, &node, outNodes, jsonNode["children"], nodesData);
        }
    } 
}

// ***********************************************************************

Scene* Scene::LoadScene(const char* filePath)
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
    ParseNodesRecursively(pScene, nullptr, pScene->m_nodes, parsed["scenes"][0]["nodes"], parsed["nodes"]);

    return pScene;
}