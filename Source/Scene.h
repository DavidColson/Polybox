#pragma once

#include <vector>
#include <map>

#include "Mesh.h"

struct Node
{
    // Tree data
    Node* m_pParent{ nullptr };
    std::vector<Node*> m_children;

    // User table info
    std::string m_name;
    uint32_t m_meshId;

    // Transform
    Vec3f m_translation;
    Vec3f m_scale;
    Quatf m_rotation;

    Matrixf m_worldTransform;
};

struct Scene
{
    std::vector<Node> m_nodes;
};

Scene* LoadScene(const char* filePath);