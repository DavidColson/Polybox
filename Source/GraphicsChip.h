#pragma once

#include "Core/Vec3.h"
#include "Core/Matrix.h"

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

enum class MatrixMode
{
    Model,
    View,
    Projection,
    Count
};

class GraphicsChip
{
public:
    void Init();

    void BeginObject(PrimitiveType type);
    void EndObject();
    void Vertex(Vec3f vec);

    // Transforms
    void SetMatrixMode(MatrixMode mode);
    void Perspective(float screenWidth, float screenHeight, float nearPlane, float farPlane, float fov);
    void Translate(Vec3f translation);
    void Rotate(Vec3f rotation);
    void Scale(Vec3f scaling);
    void Identity();

private:
    // Drawing state
    PrimitiveType m_typeState;
    std::vector<Vec3f> m_vertexState;
    MatrixMode m_matrixModeState;
    Matrixf m_matrixStates[(size_t)MatrixMode::Count];
    
    bgfx::VertexLayout m_layout;
    bgfx::ProgramHandle m_program;
};