#pragma once

#include "GraphicsChip.h"
#include "LuaCommon.h"

#include <vector>

struct Primitive : public LuaObject
{
    std::vector<VertexData> m_vertices;

    uint32_t m_baseColorTexture{ UINT32_MAX };

    int GetNumVertices();
    Vec3f GetVertexPosition(int index);
    Vec4f GetVertexColor(int index);
    Vec2f GetVertexTexCoord(int index);
    Vec3f GetVertexNormal(int index);

    int GetMaterialTextureId();

    // TODO: API for modifying vertex data (i.e. allow creating meshes on the fly)
};

struct Mesh : public LuaObject
{
    virtual ~Mesh();

    std::string m_name;
    std::vector<Primitive> m_primitives;

    int GetNumPrimitives();
    Primitive* GetPrimitive(int index);

    static std::vector<Mesh*> LoadMeshes(const char* filePath);
    static std::vector<Image*> LoadTextures(const char* filePath);  
};