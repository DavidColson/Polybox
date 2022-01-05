#pragma once

#include "Core/Vec3.h"
#include "Core/Vec4.h"
#include "Core/Matrix.h"
#include "Font.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <vector>
#include <map>

#define MAX_TEXTURES 8
#define MAX_LIGHTS 3

enum class ERenderMode
{
    Mode2D,
    Mode3D,
    None
};

enum class EPrimitiveType
{
    Points,
    Triangles,
    Lines,
    LineStrip
};

enum class EMatrixMode
{
    Model,
    View,
    Projection,
    Count
};

enum class ENormalsMode
{
    Custom,
    Flat,
    Smooth
};

struct VertexData
{
    Vec3f pos;
    Vec4f col;
    Vec2f tex;
    Vec3f norm;

    VertexData() {}
    VertexData( Vec3f _pos, Vec4f _col, Vec2f _tex, Vec3f _norm) : pos(_pos), col(_col), tex(_tex), norm(_norm) {}

    bool operator==(const VertexData& other)
    {
        return pos == other.pos;
    }
};

class GraphicsChip
{
public:
    void Init();
    void DrawFrame(float w, float h);

    // Basic draw 2D
    void BeginObject2D(EPrimitiveType type);
    void EndObject2D();
    void Vertex(Vec2f vec);

    // Basic draw 3D
    void BeginObject3D(EPrimitiveType type);
    void EndObject3D();
    void Vertex(Vec3f vec);
    void Color(Vec4f col);
    void TexCoord(Vec2f tex);
    void Normal(Vec3f norm);

    void SetClearColor(Vec3f color);

    // Transforms
    void MatrixMode(EMatrixMode mode);
    void Perspective(float screenWidth, float screenHeight, float nearPlane, float farPlane, float fov);
    void Translate(Vec3f translation);
    void Rotate(Vec3f rotation);
    void Scale(Vec3f scaling);
    void Identity();

    // Texturing
    // Enable/Disable texturing
    void BindTexture(const char* texturePath);
    Vec2f GetImageSize(const char* imagePath);
    void UnbindTexture();

    // Lighting
    void NormalsMode(ENormalsMode mode);
    void EnableLighting(bool enabled);
    void Light(int id, Vec3f direction, Vec3f color);
    void Ambient(Vec3f color);

    // Depth Cueing
    void EnableFog(bool enabled);
    void SetFogStart(float start);
    void SetFogEnd(float end);
    void SetFogColor(Vec3f color);

    // Extended Graphics API
    void DrawSprite(const char* spritePath, Vec2f position);
    void DrawSpriteRect(const char* spritePath, Vec4f rect, Vec2f position);
    void DrawText(const char* text, Vec2f position, float size);
    void DrawTextEx(const char* text, Vec2f position, Vec4f color, const char* font, float size, bool antialiasing = true, float weight = 0.0f);
    
private:
    void FullScreenQuad(float _textureWidth, float _textureHeight, float _texelHalf, bool _originBottomLeft, float _depth, float _width = 1.0f, float _height = 1.0f);


    Vec2f m_targetResolution{ Vec2f(320.0f, 240.0f) };

    // Drawing state
    ERenderMode m_mode{ ERenderMode::None };
    EPrimitiveType m_typeState;
    std::vector<VertexData> m_vertexState;
    Vec4f m_vertexColorState{ Vec4f(1.0f, 1.0f, 1.0f, 1.0f) };
    Vec2f m_vertexTexCoordState{ Vec2f(0.0f, 0.0f) };
    Vec3f m_vertexNormalState{ Vec3f(0.0f, 0.0f, 0.0f) };
    
    EMatrixMode m_matrixModeState;
    Matrixf m_matrixStates[(size_t)EMatrixMode::Count];

    Vec3f m_clearColor{ Vec3f(0.25f, 0.25f, 0.25f) };

    ENormalsMode m_normalsModeState;
    bool m_lightingState{ false };
    Vec3f m_lightDirectionsStates[MAX_LIGHTS];
    Vec3f m_lightColorStates[MAX_LIGHTS];
    Vec3f m_lightAmbientState{ Vec3f(0.0f, 0.0f, 0.0f) };

    bool m_fogState{ false };
    Vec2f m_fogDepths{ Vec2f(1.0f, 10.0f) };
    Vec3f m_fogColor{ Vec3f(0.25f, 0.25f, 0.25f) };

    struct TextureData
    {
        bgfx::TextureHandle m_handle;
        int m_width;
        int m_height;
    };
    TextureData m_textureState{ BGFX_INVALID_HANDLE };
    
    // Drawing views
    bgfx::ViewId m_realWindowView{ 0 };
    bgfx::ViewId m_scene3DView{ 1 };
    bgfx::ViewId m_scene2DView{ 2 };
    bgfx::ViewId m_compositeView{ 3 };

    // Core rendering resources
    bgfx::VertexLayout m_layout;
    bgfx::ProgramHandle m_programBase3D{ BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle m_programTexturing3D{ BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle m_programBase2D{ BGFX_INVALID_HANDLE };
    
    bgfx::UniformHandle m_colorTextureSampler{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_targetResolutionUniform{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_lightingStateUniform{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_lightDirectionUniform{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_lightColorUniform{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_lightAmbientUniform{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_fogDepthsUniform{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_fogColorUniform{ BGFX_INVALID_HANDLE };

    bgfx::ProgramHandle m_fullscreenTexProgram{ BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle m_crtProgram{ BGFX_INVALID_HANDLE };
    bgfx::FrameBufferHandle m_frameBuffer3D{ BGFX_INVALID_HANDLE };
    bgfx::FrameBufferHandle m_frameBuffer2D{ BGFX_INVALID_HANDLE };
    bgfx::FrameBufferHandle m_frameBufferComposite{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_frameBufferSampler{ BGFX_INVALID_HANDLE };
    bgfx::UniformHandle m_crtDataUniform{ BGFX_INVALID_HANDLE };

    // Texture memory
    std::map<uint64_t, TextureData> m_textureCache;
    std::map<uint64_t, Font> m_fontCache;
};