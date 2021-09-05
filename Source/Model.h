#pragma once

#include "GraphicsChip.h"

#include <vector>


struct Primitive
{
    std::vector<VertexData> m_vertices;
};

struct Mesh
{
    std::string m_name;
    std::string m_texture;
    std::vector<Primitive> m_primitives;
};

struct Node
{
    std::string m_name;
    Vec3f m_translation;
    Vec3f m_scale;
    Quatf m_rotation;
    uint32_t m_meshId;
};

struct Scene
{
    Scene() {}
    Scene(std::string filePath);

    Quatf m_cameraRotation;
    Vec3f m_cameraTranslation;

    std::vector<Mesh> m_meshes;
    std::vector<Node> m_nodes;
};