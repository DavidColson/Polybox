#pragma once

#include "Core/Vec3.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <vector>

enum class PrimitiveType
{
    Points,
    Triangles,
    Lines,
    TriangleStrip,
    LineStrip
};

class GraphicsChip
{
public:
    void Init();

    void BeginObject(PrimitiveType type);
    void EndObject();

    void Vertex(Vec3f vec);

private:
    PrimitiveType m_typeState;
    std::vector<Vec3f> m_vertexState;
    
    bgfx::VertexLayout m_layout;
    bgfx::ProgramHandle m_program;
};