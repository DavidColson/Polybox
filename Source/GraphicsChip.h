#pragma once

#include "Core/Vec3.h"
#include "Core/Vec4.h"
#include "Core/Matrix.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <vector>
#include <map>

#define MAX_TEXTURES 8

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

    // Basic draw
    void BeginObject(PrimitiveType type);
    void EndObject();
    void Vertex(Vec3f vec);
    void Color(Vec4f col);
    void TexCoord(Vec2f tex);

    // Transforms
    void SetMatrixMode(MatrixMode mode);
    void Perspective(float screenWidth, float screenHeight, float nearPlane, float farPlane, float fov);
    void Translate(Vec3f translation);
    void Rotate(Vec3f rotation);
    void Scale(Vec3f scaling);
    void Identity();

    // Texturing
    void BindTexture(const char* texturePath);
    void UnbindTexture();

private:
    void ScreenSpaceQuad(float _textureWidth, float _textureHeight, float _texelHalf, bool _originBottomLeft, float _width = 1.0f, float _height = 1.0f);


    // Drawing state
    PrimitiveType m_typeState;
    std::vector<VertexData> m_vertexState;
    MatrixMode m_matrixModeState;
    Matrixf m_matrixStates[(size_t)MatrixMode::Count];
    Vec4f m_vertexColorState{ Vec4f(1.0f, 1.0f, 1.0f, 1.0f) };
    Vec2f m_vertexTexCoordState{ Vec2f(0.0f, 0.0f) };
    bgfx::TextureHandle m_textureState{ BGFX_INVALID_HANDLE };
    
    // Drawing views
    bgfx::ViewId m_realWindowView{ 0 };
    bgfx::ViewId m_virtualWindowView{ 1 };

    // Core rendering resources
    bgfx::VertexLayout m_layout;
    bgfx::ProgramHandle m_programBase{ BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle m_programTexturing{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_colorTextureSampler{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_targetResolutionUniform{ BGFX_INVALID_HANDLE };

    bgfx::ProgramHandle m_fullscreenTexProgram{ BGFX_INVALID_HANDLE };
    bgfx::FrameBufferHandle m_frameBuffer{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_frameBufferSampler{ BGFX_INVALID_HANDLE };

    // Texture memory
    std::map<uint64_t, bgfx::TextureHandle> m_textureCache;
};