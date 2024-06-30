// Copyright 2020-2024 David Colson. All rights reserved.

#include "graphics_chip_sokol.h"

#include "font.h"
#include "image.h"
#include "sokol_impl.h"

#include <sokol_gfx.h>
#include <resizable_array.inl>
#include <maths.h>
#include <matrix.inl>
#include <vec3.inl>
#include <vec2.inl>

// shaders
#include "core3d.h"

// please excuse the dirty trick to block the windows headers messing up my API
#define DrawTextExA 0 

// this is derived from the ps1's supposed 90k poly's per second with lighting and mapping
// probably will want to increase this at some point
#define MAX_VERTICES_PER_FRAME 9000 

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

	// sokol backend stuff
	sg_pass_action passAction;
	sg_shader shaderCore3d;
	sg_bindings bind;
	sg_pipeline pipeline;
};

namespace {
	RenderState* pState;
}

// ***********************************************************************

void GraphicsInit(SDL_Window* pWindow, i32 winWidth, i32 winHeight) {
	pState = (RenderState*)g_Allocator.Allocate(sizeof(RenderState));
    SYS_P_NEW(pState) RenderState();

    pState->defaultFont = Font("assets/Roboto-Bold.ttf");

	// init_backend stuff
	GraphicsBackendInit(pWindow, winWidth, winHeight);
	sg_desc desc = {
		.environment = SokolGetEnvironment()
	};
	sg_setup(&desc);

	pState->passAction.colors[0] = { 
		.load_action = SG_LOADACTION_CLEAR,
		.clear_value = { 0.25f, 0.0f, 0.25f, 1.0f }
	};
	
	pState->shaderCore3d = sg_make_shader(core3d_shader_desc(SG_BACKEND_D3D11));

	sg_buffer_desc vertexBufferDesc = {
		.size = MAX_VERTICES_PER_FRAME * sizeof(VertexData),
		.usage = SG_USAGE_STREAM
	};
	pState->bind.vertex_buffers[0] = sg_make_buffer(&vertexBufferDesc);

	sg_buffer_desc indexBufferDesc = {
		.size = MAX_VERTICES_PER_FRAME * sizeof(u16),
		.type = SG_BUFFERTYPE_INDEXBUFFER,
		.usage = SG_USAGE_STREAM,
	};
	pState->bind.vertex_buffers[0] = sg_make_buffer(&indexBufferDesc);

	sg_pipeline_desc pipelineDesc = {
		.shader = pState->shaderCore3d,
		.layout = {
			.buffers = { {.stride = sizeof(VertexData) } },
			.attrs = {
				{ .offset = offsetof(VertexData, pos), .format = SG_VERTEXFORMAT_FLOAT3 },
				{ .offset = offsetof(VertexData, col), .format = SG_VERTEXFORMAT_FLOAT4 },
				{ .offset = offsetof(VertexData, tex), .format = SG_VERTEXFORMAT_FLOAT2 },
				{ .offset = offsetof(VertexData, norm), .format = SG_VERTEXFORMAT_FLOAT3 }
			}
		},
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true
		},
		.index_type = SG_INDEXTYPE_NONE,
		.cull_mode = SG_CULLMODE_BACK
	};
}

// ***********************************************************************

void DrawFrame(i32 w, i32 h) {
	// Present swapchain
	sg_pass pass = { .action = pState->passAction, .swapchain = SokolGetSwapchain() };
	sg_begin_pass(&pass);
	sg_end_pass();
	sg_commit();
	SokolPresent();	
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

	// Notes
	// SG_USAGE_STREAM is what we need to emulate a transient vertex buffer
	// we can only update the buffer once per frame, but we can append multiple times
	// We can also use only part of the buffer, so we presumably will have a max verts, like we had in bgfx transient buffers

	// when binding, the vertex buffer offsets will allow us to bind a specific section of our one vertex buffer
	// It'll effectively be, append to vertex buffer what you want to use, marking the start point as your offset and bind that

	// One issue is that the way we've architected the API, we will submit a draw call before we've filled the buffer
	// So we'll have to change it, and rebind it next time anyway
	// Unless of course end object 3D doesn't submit draw calls, it just fills the buffer and a command list, similar to how imgui works
	// That's maybe not a bad idea
	// This is effectively what bgfx did anyway, it just did this deferring for us
	// You actually can see in the sokol imgui sample how it minimises state changes by not rebinding if the texture hasn't changed and so on.
	// Again, something that bgfx did for us
	// Seems like we're just going to copy the imgui rendering backend then lol
	// Without the concept of a draw list I suppose
	// Though we could for example group all the 2D stuff into one vertex buffer, and all the 3D into another


	// Fresh news
	// You actually can avoid doing the above, you can interleave appends with drawcalls, as is seen here: https://github.com/floooh/sokol-samples/blob/b40a6e457c908899456a0b5c9519c16e7981ed3d/d3d11/imgui-d3d11.cc#L237
	// This is very simple for us, but worth noting that it means the user of the API is fully in control of the ordering of draw calls
	// It might be worth us storing the pipeline and bindings between calls of EndObject, so if the user does draw in an order that reduces state changes
	// They'll get a perf improvement. Not that it will matter that much though
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
