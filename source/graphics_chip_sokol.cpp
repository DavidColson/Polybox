// Copyright 2020-2024 David Colson. All rights reserved.

#include "graphics_chip_sokol.h"

#include "font.h"
#include "image.h"
#include "sokol_impl.h"

#include <sokol_gfx.h>
#include <resizable_array.inl>
#include <maths.h>
#include <matrix.inl>
#include <vec4.inl>
#include <vec3.inl>
#include <vec2.inl>
#include <defer.h>

// shaders
#include "core3d.h"

// please excuse the dirty trick to block the windows headers messing up my API
#define DrawTextExA 0 

// this is derived from the ps1's supposed 90k poly's per second with lighting and mapping
// probably will want to increase this at some point
#define MAX_VERTICES_PER_FRAME 9000 

struct DrawCommand {
	i32 vertexBufferOffset;	
	i32 indexBufferOffset;	
	i32 numVerts;
	bool indexedDraw;
	i16 textureID;
	EPrimitiveType type;
	vs_params_t uniforms;
};

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
	Vec4f lightDirectionsStates[MAX_LIGHTS];
	Vec4f lightColorStates[MAX_LIGHTS];
	Vec3f lightAmbientState { Vec3f(0.0f, 0.0f, 0.0f) };

	bool fogState { false };
	Vec2f fogDepths { Vec2f(1.0f, 10.0f) };
	Vec3f fogColor { Vec3f(0.25f, 0.25f, 0.25f) };

	Image* pTextureState;

	Font defaultFont;

	ResizableArray<DrawCommand> drawList;
	ResizableArray<VertexData> perFrameVertexBuffer;
	ResizableArray<u16> perFrameIndexBuffer;

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
	// TODO: you can't bind an index buffer and not use it, need two different binds
	//pState->bind.index_buffer = sg_make_buffer(&indexBufferDesc);

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
		.colors = {
			{
				.write_mask = SG_COLORMASK_RGB,
				.blend = { 
					.enabled = true,
					.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
					.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				},
			}
		},
		.index_type = SG_INDEXTYPE_NONE,
		.cull_mode = SG_CULLMODE_NONE
	};
	pState->pipeline = sg_make_pipeline(pipelineDesc);

	pState->perFrameVertexBuffer.Reserve(MAX_VERTICES_PER_FRAME);
	pState->perFrameIndexBuffer.Reserve(MAX_VERTICES_PER_FRAME);

	for (usize i = 0; i < 3; i++) {
        pState->matrixStates[i] = Matrixf::Identity();
    }
}

// ***********************************************************************

void DrawFrame(i32 w, i32 h) {
	// TODO: Sort the draw list to minimise state changes

	// Present swapchain
	sg_pass pass = { .action = pState->passAction, .swapchain = SokolGetSwapchain() };
	sg_begin_pass(&pass);
	sg_apply_pipeline(pState->pipeline);

	sg_range vtxData;
	vtxData.ptr = (void*)pState->perFrameVertexBuffer.pData;
	vtxData.size = pState->perFrameVertexBuffer.count * sizeof(VertexData);
	sg_update_buffer(pState->bind.vertex_buffers[0], &vtxData);

	sg_apply_viewport(0, 0, 320, 240, true);
	sg_apply_scissor_rect(0, 0, 320, 240, true);

	for(i32 i = 0; i < pState->drawList.count; i++) {

		DrawCommand& cmd = pState->drawList[i];
		if (cmd.indexedDraw) // skip for now, one thing at a time
			continue;

		sg_range uniforms = SG_RANGE_REF(cmd.uniforms);
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &uniforms);
		pState->bind.vertex_buffer_offsets[0] = cmd.vertexBufferOffset;
		sg_apply_bindings(&pState->bind);
		sg_draw(0, cmd.numVerts, 1); 
	}
	sg_end_pass();
	sg_commit();
	SokolPresent();	


	// prepare for next frame
	pState->perFrameVertexBuffer.count = 0;
	pState->perFrameIndexBuffer.count = 0;
	pState->drawList.count=0;

	for (usize i = 0; i < (int)EMatrixMode::Count; i++) {
        pState->matrixStates[i] = Matrixf::Identity();
    }
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
    if (pState->mode == ERenderMode::None)  // TODO Call errors when this is incorrect
        return;

	DrawCommand cmd;

	i32 numIndices;
	cmd.type = pState->typeState;
	bool buffersFilled = false;
    switch (pState->typeState) {
        case EPrimitiveType::Points:
		case EPrimitiveType::Lines:
		case EPrimitiveType::LineStrip:
			break;
        case EPrimitiveType::Triangles: {
            if (pState->normalsModeState == ENormalsMode::Flat) {
                for (size i = 0; i < pState->vertexState.count; i += 3) {
                    Vec3f v1 = pState->vertexState[i + 1].pos - pState->vertexState[i].pos;
                    Vec3f v2 = pState->vertexState[i + 2].pos - pState->vertexState[i].pos;
                    Vec3f faceNormal = Vec3f::Cross(v1, v2).GetNormalized();

                    pState->vertexState[i].norm = faceNormal;
                    pState->vertexState[i + 1].norm = faceNormal;
                    pState->vertexState[i + 2].norm = faceNormal;
                }

				u32 numVertices = (u32)pState->vertexState.count;
				VertexData* pDestBuffer = pState->perFrameVertexBuffer.pData + pState->perFrameVertexBuffer.count;
				if (pState->perFrameVertexBuffer.count + numVertices > MAX_VERTICES_PER_FRAME)
					return;
				memcpy(pDestBuffer, pState->vertexState.pData, numVertices * sizeof(VertexData));

				cmd.vertexBufferOffset = pState->perFrameVertexBuffer.count * sizeof(VertexData);
				cmd.numVerts = numVertices;
				pState->perFrameVertexBuffer.count += numVertices;
				buffersFilled = true;
            } else if (pState->normalsModeState == ENormalsMode::Smooth) {
				// the purpose of this is to make same vertices share the same normal vector that gets averaged from the nearby polygons
                // Convert to indexed list, loop through, saving verts into vector, each new one you search for in vector, if you find it, save index in index list.
                ResizableArray<VertexData> uniqueVerts;
                defer(uniqueVerts.Free());
                ResizableArray<u16> indices;
                defer(indices.Free());
                for (size i = 0; i < pState->vertexState.count; i++) {
                    VertexData* pVertData = uniqueVerts.Find(pState->vertexState[i]);
                    if (pVertData == uniqueVerts.end()) {
                        // New vertex
                        uniqueVerts.PushBack(pState->vertexState[i]);
                        indices.PushBack((u16)uniqueVerts.count - 1);
                    } else {
                        indices.PushBack((u16)uniqueVerts.IndexFromPointer(pVertData));
                    }
                }

                // Then run your flat shading algo on the list of vertices looping through index list. If you have a new normal for a vert, then average with the existing one
                for (size i = 0; i < indices.count; i += 3) {
                    Vec3f v1 = uniqueVerts[indices[i + 1]].pos - uniqueVerts[indices[i]].pos;
                    Vec3f v2 = uniqueVerts[indices[i + 2]].pos - uniqueVerts[indices[i]].pos;
                    Vec3f faceNormal = Vec3f::Cross(v1, v2);

                    uniqueVerts[indices[i]].norm += faceNormal;
                    uniqueVerts[indices[i + 1]].norm += faceNormal;
                    uniqueVerts[indices[i + 2]].norm += faceNormal;
                }

                for (size i = 0; i < uniqueVerts.count; i++) {
                    uniqueVerts[i].norm = uniqueVerts[i].norm.GetNormalized();
                }

                // fill vertex buffer
				u32 numVertices = (u32)uniqueVerts.count;
				VertexData* pDestBuffer = pState->perFrameVertexBuffer.pData + pState->perFrameVertexBuffer.count;
				if (pState->perFrameVertexBuffer.count + numVertices > MAX_VERTICES_PER_FRAME)
					return;
				memcpy(pDestBuffer, uniqueVerts.pData, numVertices * sizeof(VertexData));
				cmd.vertexBufferOffset = pState->perFrameVertexBuffer.count * sizeof(VertexData);
				cmd.numVerts = numVertices;
				pState->perFrameVertexBuffer.count += numVertices;

                // fill index buffer
                numIndices = (u32)indices.count;
				u16* pDestIndexBuffer = pState->perFrameIndexBuffer.pData + pState->perFrameIndexBuffer.count;
				if (pState->perFrameIndexBuffer.count + numIndices > MAX_VERTICES_PER_FRAME)
					return;
				memcpy(pDestIndexBuffer, indices.pData, numIndices * sizeof(u16));
				cmd.indexBufferOffset = pState->perFrameIndexBuffer.count * sizeof(u16);
				pState->perFrameIndexBuffer.count += numIndices;
				buffersFilled = true;
            }
        }
        default:
            break;
    }

    if (buffersFilled == false) {
        // fill vertex buffer
		u32 numVertices = (u32)pState->vertexState.count;
		VertexData* pDestBuffer = pState->perFrameVertexBuffer.pData + pState->perFrameVertexBuffer.count;
		if (pState->perFrameVertexBuffer.count + numVertices > MAX_VERTICES_PER_FRAME)
			return;
		memcpy(pDestBuffer, pState->vertexState.pData, numVertices * sizeof(VertexData));
		cmd.vertexBufferOffset = pState->perFrameVertexBuffer.count * sizeof(VertexData);
		cmd.numVerts = numVertices;
		pState->perFrameVertexBuffer.count += numVertices;
    }

    // Submit draw call
	cmd.uniforms.mvp = pState->matrixStates[(usize)EMatrixMode::Projection] * pState->matrixStates[(usize)EMatrixMode::View] * pState->matrixStates[(usize)EMatrixMode::Model];
	cmd.uniforms.model = pState->matrixStates[(usize)EMatrixMode::Model];
	cmd.uniforms.u_lightingEnabled = (i32)pState->lightingState;
	cmd.uniforms.u_lightDirection[0] = pState->lightDirectionsStates[0];
	cmd.uniforms.u_lightDirection[1] = pState->lightDirectionsStates[1];
	cmd.uniforms.u_lightDirection[2] = pState->lightDirectionsStates[2];
	cmd.uniforms.u_lightColor[0] = pState->lightColorStates[0];
	cmd.uniforms.u_lightColor[1] = pState->lightColorStates[1];
	cmd.uniforms.u_lightColor[2] = pState->lightColorStates[2];
	cmd.uniforms.u_lightAmbient = pState->lightAmbientState;

    if (pState->normalsModeState == ENormalsMode::Smooth)
        cmd.indexedDraw = true;
	else
		cmd.indexedDraw = false;

    if (pState->pTextureState) {
		// TODO: set texture ID
		pState->drawList.PushBack(cmd);
    } else {
		pState->drawList.PushBack(cmd);
    }

	// TODO: can probably reuse this memory, don't need to free
    pState->vertexState.Free();
    pState->vertexColorState = Vec4f(1.0f);
    pState->vertexTexCoordState = Vec2f();
    pState->vertexNormalState = Vec3f();
    pState->mode = ERenderMode::None;
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

    pState->lightDirectionsStates[id] = Vec4f::Embed3D(direction);
    pState->lightColorStates[id] = Vec4f::Embed3D(color);
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
