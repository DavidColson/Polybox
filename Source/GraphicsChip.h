#pragma once

#include "Core/Vec3.h"
#include "Core/Vec4.h"
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

struct VertexData
{
    Vec3f pos;
    Vec4f col;
    Vec2f tex;
};

class GraphicsChip
{
public:
    void Init();
    void DrawFrame(float w, float h);

    void BeginObject(PrimitiveType type);
    void EndObject();
    void Vertex(Vec3f vec);
    void Color(Vec4f col);

    // Transforms
    void SetMatrixMode(MatrixMode mode);
    void Perspective(float screenWidth, float screenHeight, float nearPlane, float farPlane, float fov);
    void Translate(Vec3f translation);
    void Rotate(Vec3f rotation);
    void Scale(Vec3f scaling);
    void Identity();

private:
    void ScreenSpaceQuad(float _textureWidth, float _textureHeight, float _texelHalf, bool _originBottomLeft, float _width = 1.0f, float _height = 1.0f);


    // Drawing state
    PrimitiveType m_typeState;
    std::vector<VertexData> m_vertexState;
    MatrixMode m_matrixModeState;
    Matrixf m_matrixStates[(size_t)MatrixMode::Count];
    Vec4f m_vertexColorState{ Vec4f(1.0f, 1.0f, 1.0f, 1.0f) };
    
    bgfx::ViewId m_realWindowView{ 0 };
    bgfx::ViewId m_virtualWindowView{ 1 };

    bgfx::VertexLayout m_layout;
    bgfx::ProgramHandle m_coreProgram;

    bgfx::ProgramHandle m_fullscreenTexProgram;
    bgfx::FrameBufferHandle m_frameBuffer;
    bgfx::UniformHandle m_frameBufferSampler;
};