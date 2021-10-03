#pragma once

#include "GraphicsChip.h"

#include <string>
#include <vector>


struct Primitive
{
    std::vector<VertexData> m_vertices;

    Vec4f m_baseColor{ Vec4f(1.0f) };
    uint32_t m_baseColorTexture{ UINT32_MAX };
};

struct Mesh
{
    std::string m_name;
    std::vector<Primitive> m_primitives;
};

struct Node
{
    std::string m_name;
    
    Node* m_pParent{ nullptr };
    std::vector<Node*> m_children;

    uint32_t m_meshId;

    Vec3f m_translation;
    Vec3f m_scale;
    Quatf m_rotation;

    Matrixf m_worldTransform;
};

struct Scene
{
    Scene() {}
    Scene(std::string filePath);

    Quatf m_cameraRotation;
    Vec3f m_cameraTranslation;

    std::vector<std::string> m_images;
    std::vector<Mesh> m_meshes;
    std::vector<Node> m_nodes;
};