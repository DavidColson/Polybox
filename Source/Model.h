#pragma once

#include "GraphicsChip.h"

#include <vector>


struct Primitive
{
    std::vector<VertexData> m_vertices;
};

struct Model
{
    std::string m_name;
    Vec3f m_translation;
    Quatf m_rotation;
    std::vector<Primitive> m_primitives;
};

struct Scene
{
    Scene() {}
    Scene(std::string filePath);

    std::vector<Model> m_models;
};