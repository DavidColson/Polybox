// Copyright 2020-2024 David Colson. All rights reserved.

#pragma once

#include <matrix.h>
#include <vec2.h>
#include <vec3.h>
#include <vec4.h>

#define MAX_TEXTURES 8
#define MAX_LIGHTS 3

enum class ERenderMode {
    Mode2D,
    Mode3D,
    None
};

enum class EPrimitiveType : i32 {
    Points,
    Triangles,
    TriangleStrip,
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
struct SDL_Window;
struct Font;

void GraphicsInit(SDL_Window* pWindow, i32 winWidth, i32 winHeight);
void DrawFrame(i32 w, i32 h);

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

void FullScreenQuad(f32 _textureWidth, f32 _textureHeight, f32 _texelHalf, bool _originBottomLeft, f32 _depth, f32 _width = 1.0f, f32 _height = 1.0f);
