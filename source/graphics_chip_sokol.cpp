// Copyright 2020-2024 David Colson. All rights reserved.

#include "graphics_chip_sokol.h"

#include "font.h"
#include "image.h"

#include <resizable_array.inl>
#include <maths.h>
#include <matrix.inl>
#include <vec3.inl>
#include <vec2.inl>

struct RenderState {
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

	Font defaultFont;
};

namespace {
	RenderState* pState;
}

// ***********************************************************************

void GraphicsInit(SDL_Window* pWindow) {
	pState = (RenderState*)g_Allocator.Allocate(sizeof(RenderState));
    SYS_P_NEW(pState) RenderState();

    pState->defaultFont = Font("assets/Roboto-Bold.ttf");
	// init_backend stuff
	// create sokol desc
}

// ***********************************************************************

void DrawFrame(f32 w, f32 h) {

	// Present swapchain
}

// ***********************************************************************

void BeginObject2D(EPrimitiveType type) {
    pState->typeState = type;
    pState->mode = ERenderMode::Mode2D;
}

// ***********************************************************************

void EndObject2D() {

}

// ***********************************************************************

void BeginObject3D(EPrimitiveType type) {
    // Set draw topology type
    pState->typeState = type;
    pState->mode = ERenderMode::Mode3D;
}

// ***********************************************************************

void EndObject3D() {

}

// ***********************************************************************

void Vertex(Vec3f vec) {
    pState->vertexState.PushBack({ vec, pState->vertexColorState, pState->vertexTexCoordState, pState->vertexNormalState });
}

// ***********************************************************************

void Vertex(Vec2f vec) {
    pState->vertexState.PushBack({ Vec3f::Embed2D(vec), pState->vertexColorState, pState->vertexTexCoordState, Vec3f() });
}

// ***********************************************************************

void Color(Vec4f col) {
    pState->vertexColorState = col;
}

// ***********************************************************************

void TexCoord(Vec2f tex) {
    pState->vertexTexCoordState = tex;
}

// ***********************************************************************

void Normal(Vec3f norm) {
    pState->vertexNormalState = norm;
}

// ***********************************************************************

void SetClearColor(Vec4f color) {
    pState->clearColor = color;
}

// ***********************************************************************

void MatrixMode(EMatrixMode mode) {
    pState->matrixModeState = mode;
}

// ***********************************************************************

void Perspective(f32 screenWidth, f32 screenHeight, f32 nearPlane, f32 farPlane, f32 fov) {
    pState->matrixStates[(usize)pState->matrixModeState] *= Matrixf::Perspective(screenWidth, screenHeight, nearPlane, farPlane, fov);
}

// ***********************************************************************

void Translate(Vec3f translation) {
    pState->matrixStates[(usize)pState->matrixModeState] *= Matrixf::MakeTranslation(translation);
}

// ***********************************************************************

void Rotate(Vec3f rotation) {
    pState->matrixStates[(usize)pState->matrixModeState] *= Matrixf::MakeRotation(rotation);
}

// ***********************************************************************

void Scale(Vec3f scaling) {
    pState->matrixStates[(usize)pState->matrixModeState] *= Matrixf::MakeScale(scaling);
}

// ***********************************************************************

void Identity() {
    pState->matrixStates[(usize)pState->matrixModeState] = Matrixf::Identity();
}

// ***********************************************************************

void BindTexture(Image* pImage) {
    if (pState->pTextureState != nullptr)
        UnbindTexture();

    // Save as current texture state for binding in endObject
    pState->pTextureState = pImage;
    pState->pTextureState->Retain();
}

// ***********************************************************************

void UnbindTexture() {
    pState->pTextureState->Release();
    pState->pTextureState = nullptr;
}

// ***********************************************************************

void NormalsMode(ENormalsMode mode) {
    pState->normalsModeState = mode;
}

// ***********************************************************************

void EnableLighting(bool enabled) {
    pState->lightingState = enabled;
}

// ***********************************************************************

void Light(int id, Vec3f direction, Vec3f color) {
    if (id > 2)
        return;

    pState->lightDirectionsStates[id] = direction;
    pState->lightColorStates[id] = color;
}

// ***********************************************************************

void Ambient(Vec3f color) {
    pState->lightAmbientState = color;
}

// ***********************************************************************

void EnableFog(bool enabled) {
    pState->fogState = enabled;
}

// ***********************************************************************

void SetFogStart(f32 start) {
    pState->fogDepths.x = start;
}

// ***********************************************************************

void SetFogEnd(f32 end) {
    pState->fogDepths.y = end;
}

// ***********************************************************************

void SetFogColor(Vec3f color) {
    pState->fogColor = color;
}

/*
********************************
*   EXTENDED GRAPHICS LIBRARY
********************************
*/

// ***********************************************************************

void DrawSprite(Image* pImage, Vec2f position) {
    DrawSpriteRect(pImage, Vec4f(0.0f, 0.0f, 1.0f, 1.0f), position);
}

// ***********************************************************************

void DrawSpriteRect(Image* pImage, Vec4f rect, Vec2f position) {
    f32 w = (f32)pImage->width * (rect.z - rect.x);
    f32 h = (f32)pImage->height * (rect.w - rect.y);

    Translate(Vec3f::Embed2D(position));

    BindTexture(pImage);
    BeginObject2D(EPrimitiveType::Triangles);
    TexCoord(Vec2f(rect.x, rect.w));
    Vertex(Vec2f(0.0f, 0.0f));

    TexCoord(Vec2f(rect.z, rect.w));
    Vertex(Vec2f(w, 0.0f));

    TexCoord(Vec2f(rect.z, rect.y));
    Vertex(Vec2f(w, h));

    TexCoord(Vec2f(rect.z, rect.y));
    Vertex(Vec2f(w, h));

    TexCoord(Vec2f(rect.x, rect.w));
    Vertex(Vec2f(0.0f, 0.0f));

    TexCoord(Vec2f(rect.x, rect.y));
    Vertex(Vec2f(0.f, h));
    EndObject2D();
    UnbindTexture();
}

// ***********************************************************************

void DrawText(const char* text, Vec2f position, f32 size) {
    DrawTextEx(text, position, Vec4f(1.0f, 1.0f, 1.0f, 1.0f), &pState->defaultFont, size);
}

// ***********************************************************************

void DrawTextEx(const char* text, Vec2f position, Vec4f color, Font* pFont, f32 fontSize) {
    constexpr f32 baseSize = 32.0f;

    f32 textWidth = 0.0f;
    f32 x = position.x;
    f32 y = position.y;
    Vec2f scale = Vec2f(fontSize / baseSize, fontSize / baseSize);

    String stringText(text);
    for (size i = 0; i < stringText.length; i++) {
        char c = *(stringText.pData + i);
        Character ch = pFont->characters[c];
        textWidth += ch.advance * scale.x;
    }

    BindTexture(&pFont->fontTexture);
    BeginObject2D(EPrimitiveType::Triangles);
    for (size i = 0; i < stringText.length; i++) {
        char c = *(stringText.pData + i);
        Character ch = pFont->characters[c];

        // Center alignment
        // f32 xpos = (x + ch.bearing.x * scale.x) - textWidth * 0.5f;
        f32 xpos = (x + ch.bearing.x * scale.x);
        f32 ypos = y - (ch.size.y - ch.bearing.y) * scale.y;
        f32 w = (f32)ch.size.x * scale.x;
        f32 h = (f32)ch.size.y * scale.y;

        // 0
        Color(color);
        TexCoord(Vec2f(ch.UV0.x, ch.UV1.y));
        Vertex(Vec2f(xpos, ypos));

        // 1
        Color(color);
        TexCoord(Vec2f(ch.UV1.x, ch.UV0.y));
        Vertex(Vec2f(xpos + w, ypos + h));

        // 2
        Color(color);
        TexCoord(Vec2f(ch.UV0.x, ch.UV0.y));
        Vertex(Vec2f(xpos, ypos + h));

        // 0
        Color(color);
        TexCoord(Vec2f(ch.UV0.x, ch.UV1.y));
        Vertex(Vec2f(xpos, ypos));

        // 3
        Color(color);
        TexCoord(Vec2f(ch.UV1.x, ch.UV1.y));
        Vertex(Vec2f(xpos + w, ypos));

        // 1
        Color(color);
        TexCoord(Vec2f(ch.UV1.x, ch.UV0.y));
        Vertex(Vec2f(xpos + w, ypos + h));

        x += ch.advance * scale.x;
    }
    EndObject2D();
    UnbindTexture();
}

// ***********************************************************************

void DrawPixel(Vec2f position, Vec4f color) {
    BeginObject2D(EPrimitiveType::Points);
    Color(color);
    Vertex(position);
    EndObject2D();
}

// ***********************************************************************

void DrawLine(Vec2f start, Vec2f end, Vec4f color) {
    BeginObject2D(EPrimitiveType::Lines);
    Color(color);
    Vertex(start);
    Vertex(end);
    EndObject2D();
}

// ***********************************************************************

void DrawRectangle(Vec2f bottomLeft, Vec2f topRight, Vec4f color) {
    BeginObject2D(EPrimitiveType::Triangles);
    Color(color);
    Vertex(bottomLeft);
    Vertex(Vec2f(topRight.x, bottomLeft.y));
    Vertex(topRight);

    Vertex(topRight);
    Vertex(Vec2f(bottomLeft.x, topRight.y));
    Vertex(bottomLeft);
    EndObject2D();
}

// ***********************************************************************

void DrawRectangleOutline(Vec2f bottomLeft, Vec2f topRight, Vec4f color) {
    BeginObject2D(EPrimitiveType::Lines);
    Color(color);
    Vertex(Vec2f(bottomLeft.x + 1, bottomLeft.y));
    Vertex(Vec2f(topRight.x, bottomLeft.y));

    Vertex(Vec2f(topRight.x, bottomLeft.y));
    Vertex(Vec2f(topRight.x, topRight.y - 1));

    Vertex(topRight);
    Vertex(Vec2f(bottomLeft.x + 1, topRight.y - 1));

    Vertex(bottomLeft);
    Vertex(Vec2f(bottomLeft.x + 1, topRight.y));
    EndObject2D();
}

// ***********************************************************************

void DrawCircle(Vec2f center, f32 radius, Vec4f color) {
    BeginObject2D(EPrimitiveType::Triangles);
    constexpr f32 segments = 24;
    for (usize i = 0; i < segments; i++) {
        f32 x1 = (PI2 / segments) * i;
        f32 x2 = (PI2 / segments) * (i + 1);
        Color(color);
        Vertex(center);
        Vertex(center + Vec2f(sinf(x1), cosf(x1)) * radius);
        Vertex(center + Vec2f(sinf(x2), cosf(x2)) * radius);
    }
    EndObject2D();
}

// ***********************************************************************

void DrawCircleOutline(Vec2f center, f32 radius, Vec4f color) {
    BeginObject2D(EPrimitiveType::Lines);
    constexpr f32 segments = 24;
    for (usize i = 0; i < segments; i++) {
        f32 x1 = (PI2 / segments) * i;
        f32 x2 = (PI2 / segments) * (i + 1);
        Color(color);
        Vertex(center + Vec2f(sinf(x1), cosf(x1)) * radius);
        Vertex(center + Vec2f(sinf(x2), cosf(x2)) * radius);
    }
    EndObject2D();
}
