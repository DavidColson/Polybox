// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "Font.h"

#include <bgfx/bgfx.h>
#include <matrix.h>
#include <resizable_array.h>
#include <vec3.h>
#include <vec4.h>

#define MAX_TEXTURES 8
#define MAX_LIGHTS 3

enum class ERenderMode {
    Mode2D,
    Mode3D,
    None
};

enum class EPrimitiveType {
    Points,
    Triangles,
    Lines,
    LineStrip,
    Count
};

enum class EMatrixMode {
    Model,
    View,
    Projection,
    Count
};

enum class ENormalsMode {
    Custom,
    Flat,
    Smooth
};

struct VertexData {
    Vec3f pos;
    Vec4f col;
    Vec2f tex;
    Vec3f norm;

    VertexData() {}
    VertexData(Vec3f pos, Vec4f col, Vec2f tex, Vec3f norm)
        : pos(pos), col(col), tex(tex), norm(norm) {}

    bool operator==(const VertexData& other) {
        return pos == other.pos;
    }
};

struct Image;

class GraphicsChip {
   public:
    void Init();
    void DrawFrame(f32 w, f32 h);

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

    void SetClearColor(Vec4f color);

    // Transforms
    void MatrixMode(EMatrixMode mode);
    void Perspective(f32 screenWidth, f32 screenHeight, f32 nearPlane, f32 farPlane, f32 fov);
    void Translate(Vec3f translation);
    void Rotate(Vec3f rotation);
    void Scale(Vec3f scaling);
    void Identity();

    // Texturing
    // Enable/Disable texturing
    void BindTexture(Image* pImage);
    void UnbindTexture();

    // Lighting
    void NormalsMode(ENormalsMode mode);
    void EnableLighting(bool enabled);
    void Light(int id, Vec3f direction, Vec3f color);
    void Ambient(Vec3f color);

    // Depth Cueing
    void EnableFog(bool enabled);
    void SetFogStart(f32 start);
    void SetFogEnd(f32 end);
    void SetFogColor(Vec3f color);

    // Extended Graphics API
    void DrawSprite(Image* pImage, Vec2f position);
    void DrawSpriteRect(Image* pImage, Vec4f rect, Vec2f position);
    void DrawText(const char* text, Vec2f position, f32 size);
    void DrawTextEx(const char* text, Vec2f position, Vec4f color, Font* pFont, f32 size);
    void DrawPixel(Vec2f position, Vec4f color);
    void DrawLine(Vec2f start, Vec2f end, Vec4f color);
    void DrawCircle(Vec2f center, f32 radius, Vec4f color);
    void DrawCircleOutline(Vec2f center, f32 radius, Vec4f color);
    void DrawRectangle(Vec2f bottomLeft, Vec2f topRight, Vec4f color);
    void DrawRectangleOutline(Vec2f bottomLeft, Vec2f topRight, Vec4f color);

   private:
    void FullScreenQuad(f32 _textureWidth, f32 _textureHeight, f32 _texelHalf, bool _originBottomLeft, f32 _depth, f32 _width = 1.0f, f32 _height = 1.0f);


    Vec2f targetResolution { Vec2f(320.0f, 240.0f) };

    // Drawing state
    ERenderMode mode { ERenderMode::None };
    EPrimitiveType typeState;
    ResizableArray<VertexData> vertexState;
    Vec4f vertexColorState { Vec4f(1.0f, 1.0f, 1.0f, 1.0f) };
    Vec2f vertexTexCoordState { Vec2f(0.0f, 0.0f) };
    Vec3f vertexNormalState { Vec3f(0.0f, 0.0f, 0.0f) };

    EMatrixMode matrixModeState;
    Matrixf matrixStates[(usize)EMatrixMode::Count];

    Vec4f clearColor { Vec4f(0.25f, 0.25f, 0.25f, 1.0f) };

    ENormalsMode normalsModeState;
    bool lightingState { false };
    Vec3f lightDirectionsStates[MAX_LIGHTS];
    Vec3f lightColorStates[MAX_LIGHTS];
    Vec3f lightAmbientState { Vec3f(0.0f, 0.0f, 0.0f) };

    bool fogState { false };
    Vec2f fogDepths { Vec2f(1.0f, 10.0f) };
    Vec3f fogColor { Vec3f(0.25f, 0.25f, 0.25f) };

    Image* pTextureState;

    // Drawing views
    bgfx::ViewId realWindowView { 0 };
    bgfx::ViewId scene3DView { 1 };
    bgfx::ViewId scene2DView { 2 };
    bgfx::ViewId compositeView { 3 };

    // Core rendering resources
    bgfx::VertexLayout layout;
    bgfx::ProgramHandle programBase3D { BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle programTexturing3D { BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle programBase2D { BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle programTexturing2D { BGFX_INVALID_HANDLE };

    bgfx::UniformHandle colorTextureSampler { BGFX_INVALID_HANDLE };
    bgfx::UniformHandle targetResolutionUniform { BGFX_INVALID_HANDLE };
    bgfx::UniformHandle lightingStateUniform { BGFX_INVALID_HANDLE };
    bgfx::UniformHandle lightDirectionUniform { BGFX_INVALID_HANDLE };
    bgfx::UniformHandle lightColorUniform { BGFX_INVALID_HANDLE };
    bgfx::UniformHandle lightAmbientUniform { BGFX_INVALID_HANDLE };
    bgfx::UniformHandle fogDepthsUniform { BGFX_INVALID_HANDLE };
    bgfx::UniformHandle fogColorUniform { BGFX_INVALID_HANDLE };

    bgfx::ProgramHandle fullscreenTexProgram { BGFX_INVALID_HANDLE };
    bgfx::ProgramHandle crtProgram { BGFX_INVALID_HANDLE };
    bgfx::FrameBufferHandle frameBuffer3D { BGFX_INVALID_HANDLE };
    bgfx::FrameBufferHandle frameBuffer2D { BGFX_INVALID_HANDLE };
    bgfx::FrameBufferHandle frameBufferComposite { BGFX_INVALID_HANDLE };
    bgfx::UniformHandle frameBufferSampler { BGFX_INVALID_HANDLE };
    bgfx::UniformHandle crtDataUniform { BGFX_INVALID_HANDLE };

    Font defaultFont;
};
