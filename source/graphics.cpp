// Copyright 2020-2024 David Colson. All rights reserved.
// shaders
#include "core3d.h"
#include "compositor.h"

// this is derived from the ps1's supposed 90k poly's per second with lighting and mapping
// probably will want to increase this at some point
#define MAX_VERTICES_PER_FRAME 9000 

struct DrawCommand {
	i32 vertexBufferOffset;	
	i32 indexBufferOffset;	
	i32 numElements;
	bool indexedDraw;
	bool texturedDraw;
	sg_cull_mode cullMode;
	sg_image texture;
	EPrimitiveType type;
	vs_core3d_params_t vsUniforms;
	fs_core3d_params_t fsUniforms;
};

struct RenderState {
	Arena* pArena;

	Vec2f targetResolution;

	// Drawing state
	ERenderMode mode { ERenderMode::None };
	EPrimitiveType typeState;
	ResizableArray<VertexData> vertexState;
	Vec4f vertexColorState { Vec4f(0.0f, 0.0f, 0.0f, 0.0f) };
	Vec2f vertexTexCoordState { Vec2f(0.0f, 0.0f) };
	Vec3f vertexNormalState { Vec3f(0.0f, 0.0f, 0.0f) };

	EMatrixMode matrixModeState;
	Stack<Matrixf> matrixStates[(u64)EMatrixMode::Count];

	ENormalsMode normalsModeState;
	bool lightingState { false };
	Vec4f lightDirectionsStates[MAX_LIGHTS];
	Vec4f lightColorStates[MAX_LIGHTS];
	Vec3f lightAmbientState { Vec3f(0.0f, 0.0f, 0.0f) };

	bool fogState { false };
	Vec2f fogDepths { Vec2f(0.0f, 0.0f) };
	Vec3f fogColor { Vec3f(0.f, 0.f, 0.f) };

	sg_image textureState;

	sg_cull_mode cullMode;

	ResizableArray<DrawCommand> drawList3D;
	ResizableArray<DrawCommand> drawList2D;
	ResizableArray<VertexData> perFrameVertexBuffer;
	ResizableArray<u16> perFrameIndexBuffer;
	
	// Sokol rendering data
	
	// pipelines
	sg_pipeline pipeCompositor;
	sg_pipeline pipeMain[2 * 2 * (i32)EPrimitiveType::Count];

	// passes
	sg_pass passCore3DScene;
	sg_pass passCore2DScene;
	sg_pass passCompositor;

	// persistent Buffers
	sg_buffer fullscreenTriangle;
	sg_buffer transientVertexBuffer;
	sg_buffer transientIndexBuffer;

	// framebuffers
	sg_image fbCore3DScene;
	sg_image fbCore2DScene;

	// samplers 
	sg_sampler samplerNearest;

	// misc
	sg_image whiteTexture;
};

RenderState* pRenderState;

// ***********************************************************************

sg_pipeline& GetPipeline(bool indexed, EPrimitiveType primitive, bool writeAlpha, sg_cull_mode cullMode) {
	u32 index = (i32)indexed + (i32)primitive + (i32)writeAlpha + (i32)cullMode;
	if (pRenderState->pipeMain[index].id != SG_INVALID_ID) {
		return pRenderState->pipeMain[index];
	}

	sg_pipeline_desc pipelineDesc = {
		.shader = sg_make_shader(core3D_shader_desc(SG_BACKEND_D3D11)),
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
			.pixel_format = SG_PIXELFORMAT_DEPTH,
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true,
		},
		.colors = {
			{
				.blend = { 
					.enabled = true,
					.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
					.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				},
			}
		},
		.index_type = SG_INDEXTYPE_NONE,
		.cull_mode = cullMode
	};

	switch(primitive) {
		case EPrimitiveType::Points:
			pipelineDesc.primitive_type = SG_PRIMITIVETYPE_POINTS;	
		break;
		case EPrimitiveType::Triangles:
			pipelineDesc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;	
		break;
		case EPrimitiveType::TriangleStrip:
			pipelineDesc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;	
		break;
		case EPrimitiveType::Lines:
			pipelineDesc.primitive_type = SG_PRIMITIVETYPE_LINES;	
		break;
		case EPrimitiveType::LineStrip:
			pipelineDesc.primitive_type = SG_PRIMITIVETYPE_LINE_STRIP;	
		break;
		default:
			pipelineDesc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;	
		break;
	}

	if (indexed) {
		pipelineDesc.index_type = SG_INDEXTYPE_UINT16;
	} else {
		pipelineDesc.index_type = SG_INDEXTYPE_NONE;
	}

	if (writeAlpha) {
		pipelineDesc.colors[0].write_mask = SG_COLORMASK_RGBA;
	} else {
		pipelineDesc.colors[0].write_mask = SG_COLORMASK_RGB;
	}

	pRenderState->pipeMain[index] = sg_make_pipeline(pipelineDesc);
	return pRenderState->pipeMain[index];
}

// ***********************************************************************

void CreateFullScreenQuad(f32 _textureWidth, f32 _textureHeight, f32 _texelHalf, bool _originBottomLeft, f32 _depth, f32 _width = 1.0f, f32 _height = 1.0f) {
	
	// allocate space for 3 vertices, stack might be fine
	VertexData vertices[3];

	const f32 minx = -_width;
	const f32 maxx = _width;
	const f32 miny = 0.0f;
	const f32 maxy = _height * 2.0f;

	const f32 texelHalfW = _texelHalf / _textureWidth;
	const f32 texelHalfH = _texelHalf / _textureHeight;
	const f32 minu = -1.0f + texelHalfW;
	const f32 maxu = 1.0f + texelHalfH;

	const f32 zz = _depth;

	f32 minv = texelHalfH;
	f32 maxv = 2.0f + texelHalfH;

	if (_originBottomLeft) {
		f32 temp = minv;
		minv = maxv;
		maxv = temp;

		minv -= 1.0f;
		maxv -= 1.0f;
	}

	vertices[0].pos.x = minx;
	vertices[0].pos.y = miny;
	vertices[0].pos.z = zz;
	vertices[0].tex.x = minu;
	vertices[0].tex.y = minv;

	vertices[1].pos.x = maxx;
	vertices[1].pos.y = maxy;
	vertices[1].pos.z = zz;
	vertices[1].tex.x = maxu;
	vertices[1].tex.y = maxv;

	vertices[2].pos.x = maxx;
	vertices[2].pos.y = miny;
	vertices[2].pos.z = zz;
	vertices[2].tex.x = maxu;
	vertices[2].tex.y = minv;

	sg_buffer_desc vbufferDesc = {
		.data = SG_RANGE(vertices)
	};
	pRenderState->fullscreenTriangle = sg_make_buffer(&vbufferDesc);
}


// ***********************************************************************

void GraphicsInit(SDL_Window* pWindow, i32 winWidth, i32 winHeight) {
	Arena* pArena = ArenaCreate();
	pRenderState = New(pArena, RenderState);
	pRenderState->pArena = pArena;

	pRenderState->vertexState.pArena = pArena;
	pRenderState->drawList3D.pArena = pArena;
	pRenderState->drawList2D.pArena = pArena;
	pRenderState->perFrameVertexBuffer.pArena = pArena;
	pRenderState->perFrameIndexBuffer.pArena = pArena;

	pRenderState->targetResolution = Vec2f(320.0f, 240.0f);

	// init_backend stuff
	GraphicsBackendInit(pWindow, winWidth, winHeight);
	sg_desc desc = {
		.environment = SokolGetEnvironment()
	};
	sg_setup(&desc);

	// Create white texture for non textured draws
	{
		u32 pixels[4];
		memset(pixels, 0xFF, sizeof(pixels));

		sg_image_desc imageDesc = {
			.type = SG_IMAGETYPE_2D,
			.width = 2,
			.height = 2,
			.pixel_format = SG_PIXELFORMAT_RGBA8,
			.label = "white"
		};

		imageDesc.data.subimage[0][0].ptr = (void*)pixels;
		imageDesc.data.subimage[0][0].size = sizeof(pixels);
		pRenderState->whiteTexture = sg_make_image(&imageDesc);
	}

	// Compositor Pipeline
	{
		sg_pipeline_desc pipelineDesc = {
			.shader = sg_make_shader(compositor_shader_desc(SG_BACKEND_D3D11)),
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
				.write_enabled = true,
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
			.cull_mode = SG_CULLMODE_BACK
		};
		pRenderState->pipeCompositor = sg_make_pipeline(pipelineDesc);
	}

	// Create persistent buffers
	{
		CreateFullScreenQuad((f32)winWidth, (f32)winHeight, 0.0f, true, 0.0f);

		sg_buffer_desc vertexBufferDesc = {
			.size = MAX_VERTICES_PER_FRAME * sizeof(VertexData),
			.usage = SG_USAGE_STREAM
		};
		pRenderState->transientVertexBuffer = sg_make_buffer(&vertexBufferDesc);

		sg_buffer_desc indexBufferDesc = {
			.size = MAX_VERTICES_PER_FRAME * sizeof(u16),
			.type = SG_BUFFERTYPE_INDEXBUFFER,
			.usage = SG_USAGE_STREAM
		};
		pRenderState->transientIndexBuffer = sg_make_buffer(&indexBufferDesc);

	}

	// Create core3D scene pass
	{
		sg_image_desc viewDesc = {
			.render_target = true,
			.width = 320,
			.height = 240,
			.sample_count = 1
		};
		pRenderState->fbCore3DScene = sg_make_image(&viewDesc);

		viewDesc.pixel_format = SG_PIXELFORMAT_DEPTH;
		sg_image depth = sg_make_image(&viewDesc);

		sg_attachments_desc attachmentsDesc = {
			.colors = { {.image = pRenderState->fbCore3DScene } },
			.depth_stencil = { .image = depth }
		};

		pRenderState->passCore3DScene = { 
			.action = {
				.colors = {
					{ .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.25f, 0.25f, 0.25f, 1.0f } } 
				}
			},
			.attachments = sg_make_attachments(&attachmentsDesc)
		};
	}

	// Create core2D scene pass
	{
		sg_image_desc viewDesc = {
			.render_target = true,
			.width = 320,
			.height = 240,
			.sample_count = 1
		};
		pRenderState->fbCore2DScene = sg_make_image(&viewDesc);

		viewDesc.pixel_format = SG_PIXELFORMAT_DEPTH;
		sg_image depth = sg_make_image(&viewDesc);

		sg_attachments_desc attachmentsDesc = {
			.colors = { {.image = pRenderState->fbCore2DScene } },
			.depth_stencil = { .image = depth }
		};

		pRenderState->passCore2DScene = { 
			.action = {
				.colors = {
					{ .load_action = SG_LOADACTION_CLEAR, .clear_value = { 1.f, 1.f, 1.f, 0.0f } } 
				}
			},
			.attachments = sg_make_attachments(&attachmentsDesc)
		};
	}

	// Create compositor
	{
		pRenderState->passCompositor = { 
			.action = {
				.colors = {
					{ .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.25f, 0.25f, 0.25f, 1.0f } } 
				}
			},
			.swapchain = SokolGetSwapchain()
		};
	}

	// Create samplers
	{
		sg_sampler_desc samplerDesc = {
			.min_filter = SG_FILTER_NEAREST,
			.mag_filter = SG_FILTER_NEAREST
		};
		pRenderState->samplerNearest = sg_make_sampler(&samplerDesc);
	}

	pRenderState->perFrameVertexBuffer.Reserve(MAX_VERTICES_PER_FRAME);
	pRenderState->perFrameIndexBuffer.Reserve(MAX_VERTICES_PER_FRAME);

	for (u64 i = 0; i < 3; i++) {
		pRenderState->matrixStates[i].array.pArena = pArena;
        pRenderState->matrixStates[i].Push(Matrixf::Identity());
	}
}

// ***********************************************************************

void DrawFrame(i32 w, i32 h) {
	// TODO: Sort the draw list to minimise state changes

	// Update the global vertex buffer
	if (pRenderState->perFrameVertexBuffer.count > 0) {
		sg_range vtxData;
		vtxData.ptr = (void*)pRenderState->perFrameVertexBuffer.pData;
		vtxData.size = pRenderState->perFrameVertexBuffer.count * sizeof(VertexData);
		sg_update_buffer(pRenderState->transientVertexBuffer, &vtxData);
	}

	if (pRenderState->perFrameIndexBuffer.count > 0) {
		sg_range idxData;
		idxData.ptr = (void*)pRenderState->perFrameIndexBuffer.pData;
		idxData.size = pRenderState->perFrameIndexBuffer.count * sizeof(u16);
		sg_update_buffer(pRenderState->transientIndexBuffer, &idxData);
	}

	// Draw 3D view into texture
	{
		sg_begin_pass(&pRenderState->passCore3DScene);

		sg_apply_viewport(0, 0, (i32)pRenderState->targetResolution.x, (i32)pRenderState->targetResolution.y, true);
		sg_apply_scissor_rect(0, 0, (i32)pRenderState->targetResolution.x, (i32)pRenderState->targetResolution.y, true);

		for(i32 i = 0; i < pRenderState->drawList3D.count; i++) {
			DrawCommand& cmd = pRenderState->drawList3D[i];

			sg_pipeline& pipeline = GetPipeline(cmd.indexedDraw, cmd.type, false, cmd.cullMode);
			sg_apply_pipeline(pipeline);
			
			sg_bindings bind{0};
			bind.vertex_buffers[0] = pRenderState->transientVertexBuffer;
			bind.fs = {
				.images = { pRenderState->whiteTexture },
				.samplers = { pRenderState->samplerNearest }
			};

			if (cmd.indexedDraw) {
				bind.index_buffer = pRenderState->transientIndexBuffer;
				bind.index_buffer_offset = cmd.indexBufferOffset;
			}

			if (cmd.texturedDraw) {
				bind.fs.images[0] = cmd.texture;
			}

			sg_range vsUniforms = SG_RANGE_REF(cmd.vsUniforms);
			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vsUniforms);
			sg_range fsUniforms = SG_RANGE_REF(cmd.fsUniforms);
			sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fsUniforms);

			bind.vertex_buffer_offsets[0] = cmd.vertexBufferOffset;
			sg_apply_bindings(&bind);
			sg_draw(0, cmd.numElements, 1); 
		}
		sg_end_pass();
	}

	// experimental code to readback 3D view for cpu editing
	// ReadbackImagePixels(pRenderState->fbCore3DScene, pRenderState->pPixelsData);
	// memset(pRenderState->pPixelsData + 8000 * 4 * sizeof(u8), 0, 640); 

	// Draw 2D view into texture
	{
		sg_begin_pass(&pRenderState->passCore2DScene);

		sg_apply_viewport(0, 0, (i32)pRenderState->targetResolution.x, (i32)pRenderState->targetResolution.y, true);
		sg_apply_scissor_rect(0, 0, (i32)pRenderState->targetResolution.x, (i32)pRenderState->targetResolution.y, true);

		for(i32 i = 0; i < pRenderState->drawList2D.count; i++) {
			DrawCommand& cmd = pRenderState->drawList2D[i];

			sg_pipeline& pipeline = GetPipeline(cmd.indexedDraw, cmd.type, true, SG_CULLMODE_NONE);
			sg_apply_pipeline(pipeline);

			sg_bindings bind{0};
			bind.vertex_buffers[0] = pRenderState->transientVertexBuffer;
			bind.fs = {
				.images = { pRenderState->whiteTexture },
				.samplers = { pRenderState->samplerNearest }
			};

			if (cmd.texturedDraw) {
				bind.fs.images[0] = cmd.texture;
			}

			Assert(!cmd.indexedDraw);

			sg_range vsUniforms = SG_RANGE_REF(cmd.vsUniforms);
			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vsUniforms);
			sg_range fsUniforms = SG_RANGE_REF(cmd.fsUniforms);
			sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fsUniforms);

			bind.vertex_buffer_offsets[0] = cmd.vertexBufferOffset;
			sg_apply_bindings(&bind);
			sg_draw(0, cmd.numElements, 1); 
		}
		sg_end_pass();
	}

	// Draw framebuffer to swapchain, upscaling in the process
	{
		sg_begin_pass(&pRenderState->passCompositor);
		sg_apply_pipeline(pRenderState->pipeCompositor);

		// paramaterize this, the true screen resolution
		sg_apply_viewport(0, 0, 1280, 720, true);
		sg_apply_scissor_rect(0, 0, 1280, 720, true);

		sg_bindings bind = { 
			.vertex_buffers = { pRenderState->fullscreenTriangle },
			.fs = {
				.images = { { pRenderState->fbCore2DScene }, { pRenderState->fbCore3DScene } },
				.samplers = { { pRenderState->samplerNearest } }
			}
		};
		sg_apply_bindings(&bind);

		vs_compositor_params_t vsUniforms;
		vsUniforms.mvp = Matrixf::Orthographic(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 100.0f);
		sg_range vsUniformsRange = SG_RANGE_REF(vsUniforms);
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vsUniformsRange);

		fs_compositor_params_t fsUniforms;
		fsUniforms.screenResolution = Vec2f(1280, 720);
		fsUniforms.time = f32(SDL_GetTicks()) / 1000.0f;
		sg_range fsUniformsRange = SG_RANGE_REF(fsUniforms);
		sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fsUniformsRange);

		sg_draw(0, 3, 1);
		sg_end_pass();
	}




	sg_commit();
	SokolPresent();	
	// prepare for next frame
	pRenderState->perFrameVertexBuffer.count = 0;
	pRenderState->perFrameIndexBuffer.count = 0;
	pRenderState->drawList3D.count=0;
	pRenderState->drawList2D.count = 0;

	for (u64 i = 0; i < (int)EMatrixMode::Count; i++) {
        pRenderState->matrixStates[i].array.count = 1;
        pRenderState->matrixStates[i][0] = Matrixf::Identity();
    }
}

// ***********************************************************************

void BeginObject2D(EPrimitiveType type) {
    pRenderState->typeState = type;
    pRenderState->mode = ERenderMode::Mode2D;
}

// ***********************************************************************

void EndObject2D() {
    if (pRenderState->mode == ERenderMode::None)  // TODO Call errors when this is incorrect
        return;

	DrawCommand cmd;
	
	cmd.type = pRenderState->typeState;

	// fill vertex buffer
	// TODO: this pattern is repeated, could refactor out into function such as FillTransientBuffer
	u32 numVertices = (u32)pRenderState->vertexState.count;
	VertexData* pDestBuffer = pRenderState->perFrameVertexBuffer.pData + pRenderState->perFrameVertexBuffer.count;
	if (pRenderState->perFrameVertexBuffer.count + numVertices > MAX_VERTICES_PER_FRAME)
		return;
	memcpy(pDestBuffer, pRenderState->vertexState.pData, numVertices * sizeof(VertexData));
	cmd.vertexBufferOffset = (i32)pRenderState->perFrameVertexBuffer.count * sizeof(VertexData);
	cmd.numElements = numVertices;
	pRenderState->perFrameVertexBuffer.count += numVertices;

    // Submit draw call
    Matrixf ortho = Matrixf::Orthographic(0.0f, pRenderState->targetResolution.x, 0.0f, pRenderState->targetResolution.y, -100.0f, 100.0f);
	cmd.vsUniforms.mvp = ortho * pRenderState->matrixStates[(u64)EMatrixMode::Model][-1];
	cmd.vsUniforms.model = pRenderState->matrixStates[(u64)EMatrixMode::Model][-1];
	cmd.vsUniforms.modelView = pRenderState->matrixStates[(u64)EMatrixMode::View][-1] * pRenderState->matrixStates[(u64)EMatrixMode::Model][-1];
	cmd.vsUniforms.lightingEnabled = 0;
	cmd.vsUniforms.lightDirection[0] = 0.f;
	cmd.vsUniforms.lightDirection[1] = 0.f;
	cmd.vsUniforms.lightDirection[2] = 0.f;
	cmd.vsUniforms.lightColor[0] = 0.f;
	cmd.vsUniforms.lightColor[1] = 0.f;
	cmd.vsUniforms.lightColor[2] = 0.f;
	cmd.vsUniforms.lightAmbient = Vec3f(0.0);
	cmd.vsUniforms.targetResolution = pRenderState->targetResolution;
	cmd.vsUniforms.fogEnabled = 0;
	cmd.vsUniforms.fogDepths = Vec2f(0.0);
	cmd.fsUniforms.fogColor = Vec4f(0.0);

	cmd.indexedDraw = false;

    if (pRenderState->textureState.id != SG_INVALID_ID) {
		cmd.texturedDraw = true;
		cmd.texture = pRenderState->textureState;
    } else {
		cmd.texturedDraw = false;
    }
	pRenderState->drawList2D.PushBack(cmd);

    pRenderState->vertexState.count = 0;
    pRenderState->vertexColorState = Vec4f(1.0f);
    pRenderState->vertexTexCoordState = Vec2f();
    pRenderState->vertexNormalState = Vec3f();
    pRenderState->mode = ERenderMode::None;
}

// ***********************************************************************

void BeginObject3D(EPrimitiveType type) {
    // Set draw topology type
    pRenderState->typeState = type;
    pRenderState->mode = ERenderMode::Mode3D;
}

// ***********************************************************************

void EndObject3D() {
    if (pRenderState->mode == ERenderMode::None)  // TODO Call errors when this is incorrect
        return;

	DrawCommand cmd;

	i32 numIndices;
	cmd.type = pRenderState->typeState;
	cmd.cullMode = pRenderState->cullMode;
	bool buffersFilled = false;
    switch (pRenderState->typeState) {
        case EPrimitiveType::Points:
		case EPrimitiveType::Lines:
		case EPrimitiveType::LineStrip:
			break;
        case EPrimitiveType::Triangles: {
            if (pRenderState->normalsModeState == ENormalsMode::Flat) {
                for (i64 i = 0; i < pRenderState->vertexState.count; i += 3) {
                    Vec3f v1 = pRenderState->vertexState[i + 1].pos - pRenderState->vertexState[i].pos;
                    Vec3f v2 = pRenderState->vertexState[i + 2].pos - pRenderState->vertexState[i].pos;
                    Vec3f faceNormal = Vec3f::Cross(v1, v2).GetNormalized();

                    pRenderState->vertexState[i].norm = faceNormal;
                    pRenderState->vertexState[i + 1].norm = faceNormal;
                    pRenderState->vertexState[i + 2].norm = faceNormal;
                }

				u32 numVertices = (u32)pRenderState->vertexState.count;
				VertexData* pDestBuffer = pRenderState->perFrameVertexBuffer.pData + pRenderState->perFrameVertexBuffer.count;
				if (pRenderState->perFrameVertexBuffer.count + numVertices > MAX_VERTICES_PER_FRAME)
					return;
				memcpy(pDestBuffer, pRenderState->vertexState.pData, numVertices * sizeof(VertexData));

				cmd.vertexBufferOffset = (i32)pRenderState->perFrameVertexBuffer.count * sizeof(VertexData);
				cmd.numElements = numVertices;
				pRenderState->perFrameVertexBuffer.count += numVertices;
				buffersFilled = true;
            } else if (pRenderState->normalsModeState == ENormalsMode::Smooth) {
				// the purpose of this is to make same vertices share the same normal vector that gets averaged from the nearby polygons
                // Convert to indexed list, loop through, saving verts into vector, each new one you search for in vector, if you find it, save index in index list.

                ResizableArray<VertexData> uniqueVerts(g_pArenaFrame);
                ResizableArray<u16> indices(g_pArenaFrame);
                for (i64 i = 0; i < pRenderState->vertexState.count; i++) {
                    VertexData* pVertData = uniqueVerts.Find(pRenderState->vertexState[i]);
                    if (pVertData == uniqueVerts.end()) {
                        // New vertex
                        uniqueVerts.PushBack(pRenderState->vertexState[i]);
                        indices.PushBack((u16)uniqueVerts.count - 1);
                    } else {
                        indices.PushBack((u16)uniqueVerts.IndexFromPointer(pVertData));
                    }
                }

                // Then run your flat shading algo on the list of vertices looping through index list. If you have a new normal for a vert, then average with the existing one
                for (i64 i = 0; i < indices.count; i += 3) {
                    Vec3f v1 = uniqueVerts[indices[i + 1]].pos - uniqueVerts[indices[i]].pos;
                    Vec3f v2 = uniqueVerts[indices[i + 2]].pos - uniqueVerts[indices[i]].pos;
                    Vec3f faceNormal = Vec3f::Cross(v1, v2);

                    uniqueVerts[indices[i]].norm += faceNormal;
                    uniqueVerts[indices[i + 1]].norm += faceNormal;
                    uniqueVerts[indices[i + 2]].norm += faceNormal;
                }

                for (i64 i = 0; i < uniqueVerts.count; i++) {
                    uniqueVerts[i].norm = uniqueVerts[i].norm.GetNormalized();
                }

                // fill vertex buffer
				u32 numVertices = (u32)uniqueVerts.count;
				VertexData* pDestBuffer = pRenderState->perFrameVertexBuffer.pData + pRenderState->perFrameVertexBuffer.count;
				if (pRenderState->perFrameVertexBuffer.count + numVertices > MAX_VERTICES_PER_FRAME)
					return;
				memcpy(pDestBuffer, uniqueVerts.pData, numVertices * sizeof(VertexData));
				cmd.vertexBufferOffset = (i32)pRenderState->perFrameVertexBuffer.count * sizeof(VertexData);
				pRenderState->perFrameVertexBuffer.count += numVertices;

                // fill index buffer
                numIndices = (u32)indices.count;
				u16* pDestIndexBuffer = pRenderState->perFrameIndexBuffer.pData + pRenderState->perFrameIndexBuffer.count;
				if (pRenderState->perFrameIndexBuffer.count + numIndices > MAX_VERTICES_PER_FRAME)
					return;
				memcpy(pDestIndexBuffer, indices.pData, numIndices * sizeof(u16));
				cmd.indexBufferOffset = (i32)pRenderState->perFrameIndexBuffer.count * sizeof(u16);
				pRenderState->perFrameIndexBuffer.count += numIndices;

				cmd.numElements = numIndices;
				buffersFilled = true;
            }
        }
        default:
            break;
    }

    if (buffersFilled == false) {
        // fill vertex buffer
		u32 numVertices = (u32)pRenderState->vertexState.count;
		VertexData* pDestBuffer = pRenderState->perFrameVertexBuffer.pData + pRenderState->perFrameVertexBuffer.count;
		if (pRenderState->perFrameVertexBuffer.count + numVertices > MAX_VERTICES_PER_FRAME)
			return;
		memcpy(pDestBuffer, pRenderState->vertexState.pData, numVertices * sizeof(VertexData));
		cmd.vertexBufferOffset = (i32)pRenderState->perFrameVertexBuffer.count * sizeof(VertexData);
		cmd.numElements = numVertices;
		pRenderState->perFrameVertexBuffer.count += numVertices;
    }

    // Submit draw call
	cmd.vsUniforms.mvp = pRenderState->matrixStates[(u64)EMatrixMode::Projection][-1] * pRenderState->matrixStates[(u64)EMatrixMode::View][-1] * pRenderState->matrixStates[(u64)EMatrixMode::Model][-1];
	cmd.vsUniforms.model = pRenderState->matrixStates[(u64)EMatrixMode::Model][-1];
	cmd.vsUniforms.modelView = pRenderState->matrixStates[(u64)EMatrixMode::View][-1] * pRenderState->matrixStates[(u64)EMatrixMode::Model][-1];
	cmd.vsUniforms.lightingEnabled = (i32)pRenderState->lightingState;
	cmd.vsUniforms.lightDirection[0] = pRenderState->lightDirectionsStates[0];
	cmd.vsUniforms.lightDirection[1] = pRenderState->lightDirectionsStates[1];
	cmd.vsUniforms.lightDirection[2] = pRenderState->lightDirectionsStates[2];
	cmd.vsUniforms.lightColor[0] = pRenderState->lightColorStates[0];
	cmd.vsUniforms.lightColor[1] = pRenderState->lightColorStates[1];
	cmd.vsUniforms.lightColor[2] = pRenderState->lightColorStates[2];
	cmd.vsUniforms.lightAmbient = pRenderState->lightAmbientState;
	cmd.vsUniforms.targetResolution = pRenderState->targetResolution;
	cmd.vsUniforms.fogEnabled = (i32)pRenderState->fogState;
	cmd.vsUniforms.fogDepths = pRenderState->fogDepths;
	cmd.fsUniforms.fogColor = Vec4f::Embed3D(pRenderState->fogColor);

    if (pRenderState->normalsModeState == ENormalsMode::Smooth && cmd.type == EPrimitiveType::Triangles)
        cmd.indexedDraw = true;
	else
		cmd.indexedDraw = false;

    if (pRenderState->textureState.id != SG_INVALID_ID) {
		cmd.texturedDraw = true;
		cmd.texture = pRenderState->textureState;
    } else {
		cmd.texturedDraw = false;
    }
	pRenderState->drawList3D.PushBack(cmd);

    pRenderState->vertexState.count = 0;
    pRenderState->vertexColorState = Vec4f(1.0f);
    pRenderState->vertexTexCoordState = Vec2f();
    pRenderState->vertexNormalState = Vec3f();
    pRenderState->mode = ERenderMode::None;
}

// ***********************************************************************

void Vertex(Vec3f vec) {
    pRenderState->vertexState.PushBack({ vec, pRenderState->vertexColorState, pRenderState->vertexTexCoordState, pRenderState->vertexNormalState });
}

// ***********************************************************************

void Vertex(Vec2f vec) {
    pRenderState->vertexState.PushBack({ Vec3f::Embed2D(vec), pRenderState->vertexColorState, pRenderState->vertexTexCoordState, Vec3f() });
}

// ***********************************************************************

void Color(Vec4f col) {
    pRenderState->vertexColorState = col;
}

// ***********************************************************************

void TexCoord(Vec2f tex) {
    pRenderState->vertexTexCoordState = tex;
}

// ***********************************************************************

void Normal(Vec3f norm) {
    pRenderState->vertexNormalState = norm;
}

// ***********************************************************************

void SetCullMode(sg_cull_mode mode) {
	pRenderState->cullMode = mode;
}

// ***********************************************************************

void SetClearColor(Vec4f color) {
	pRenderState->passCore3DScene.action.colors[0].clear_value = { color.x, color.y, color.z, color.w }; 
}

// ***********************************************************************

void MatrixMode(EMatrixMode mode) {
    pRenderState->matrixModeState = mode;
}

// ***********************************************************************

void PushMatrix() {
	u64 mode = (u64)pRenderState->matrixModeState;
	pRenderState->matrixStates[mode].Push(pRenderState->matrixStates[mode].Top());
}

// ***********************************************************************

void PopMatrix() {
	u64 mode = (u64)pRenderState->matrixModeState;
	pRenderState->matrixStates[mode].Pop();
}

// ***********************************************************************

void LoadMatrix(Matrixf mat) {
	u64 mode = (u64)pRenderState->matrixModeState;
	pRenderState->matrixStates[mode].Top() = mat;
}

// ***********************************************************************

Matrixf GetMatrix() {
	u64 mode = (u64)pRenderState->matrixModeState;
	return pRenderState->matrixStates[mode].Top();
}

// ***********************************************************************

void Perspective(f32 screenWidth, f32 screenHeight, f32 nearPlane, f32 farPlane, f32 fov) {
	u64 mode = (u64)pRenderState->matrixModeState;
    pRenderState->matrixStates[mode].Top() *= Matrixf::Perspective(screenWidth, screenHeight, nearPlane, farPlane, fov);
}

// ***********************************************************************

void Translate(Vec3f translation) {
	u64 mode = (u64)pRenderState->matrixModeState;
    pRenderState->matrixStates[mode].Top() *= Matrixf::MakeTranslation(translation);
}

// ***********************************************************************

void Rotate(float angle, Vec3f axis) {
	u64 mode = (u64)pRenderState->matrixModeState;
    pRenderState->matrixStates[mode].Top() *= Matrixf::MakeRotation(angle, axis);
}

// ***********************************************************************

void Scale(Vec3f scaling) {
	u64 mode = (u64)pRenderState->matrixModeState;
    pRenderState->matrixStates[mode].Top() *= Matrixf::MakeScale(scaling);
}

// ***********************************************************************

void Identity() {
	u64 mode = (u64)pRenderState->matrixModeState;
    pRenderState->matrixStates[mode].Top() = Matrixf::Identity();
}

// ***********************************************************************

void BindTexture(sg_image image) {
    if (pRenderState->textureState.id != SG_INVALID_ID)
        UnbindTexture();

    // Save as current texture state for binding in endObject
    pRenderState->textureState = image;
}

// ***********************************************************************

void UnbindTexture() {
    pRenderState->textureState.id = SG_INVALID_ID;
}

// ***********************************************************************

void NormalsMode(ENormalsMode mode) {
    pRenderState->normalsModeState = mode;
}

// ***********************************************************************

void EnableLighting(bool enabled) {
    pRenderState->lightingState = enabled;
}

// ***********************************************************************

void Light(int id, Vec3f direction, Vec3f color) {
    if (id > 2)
        return;

    pRenderState->lightDirectionsStates[id] = Vec4f::Embed3D(direction);
    pRenderState->lightColorStates[id] = Vec4f::Embed3D(color);
}

// ***********************************************************************

void Ambient(Vec3f color) {
    pRenderState->lightAmbientState = color;
}

// ***********************************************************************

void EnableFog(bool enabled) {
    pRenderState->fogState = enabled;
}

// ***********************************************************************

void SetFogStart(f32 start) {
    pRenderState->fogDepths.x = start;
}

// ***********************************************************************

void SetFogEnd(f32 end) {
    pRenderState->fogDepths.y = end;
}

// ***********************************************************************

void SetFogColor(Vec3f color) {
    pRenderState->fogColor = color;
}

/*
********************************
*   EXTENDED GRAPHICS LIBRARY
********************************
*/

// ***********************************************************************

void DrawSprite(sg_image image, Vec2f position) {
    DrawSpriteRect(image, Vec4f(0.0f, 0.0f, 1.0f, 1.0f), position);
}

// ***********************************************************************

void DrawSpriteRect(sg_image image, Vec4f rect, Vec2f position) {
    f32 w = (f32)(rect.z - rect.x);
    f32 h = (f32)(rect.w - rect.y);

    Translate(Vec3f::Embed2D(position));

    BindTexture(image);
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

