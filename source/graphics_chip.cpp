// Copyright 2020-2022 David Colson. All rights reserved.

#include "graphics_chip.h"

#include "image.h"

#include <types.h>
#include <matrix.inl>
#include <vec2.inl>
#include <vec3.inl>
#include <vec4.inl>
#include <SDL_rwops.h>
#include <SDL_timer.h>
#include <defer.h>
#include <bx/bx.h>
#include <resizable_array.inl>


// ***********************************************************************

u64 StringHash(const char* str) {
    u64 offset = 14695981039346656037u;
    u64 prime = 1099511628211u;

    u64 hash = offset;
    while (*str != 0) {
        hash ^= *str++;
        hash *= prime;
    }
    return hash;
}

// ***********************************************************************

void GraphicsChip::FullScreenQuad(f32 _textureWidth, f32 _textureHeight, f32 _texelHalf, bool _originBottomLeft, f32 _depth, f32 _width, f32 _height) {
    if (3 == bgfx::getAvailTransientVertexBuffer(3, layout)) {
        bgfx::TransientVertexBuffer vb;
        bgfx::allocTransientVertexBuffer(&vb, 3, layout);
        VertexData* vertex = (VertexData*)vb.data;

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

        vertex[0].pos.x = minx;
        vertex[0].pos.y = miny;
        vertex[0].pos.z = zz;
        vertex[0].tex.x = minu;
        vertex[0].tex.y = minv;

        vertex[1].pos.x = maxx;
        vertex[1].pos.y = miny;
        vertex[1].pos.z = zz;
        vertex[1].tex.x = maxu;
        vertex[1].tex.y = minv;

        vertex[2].pos.x = maxx;
        vertex[2].pos.y = maxy;
        vertex[2].pos.z = zz;
        vertex[2].tex.x = maxu;
        vertex[2].tex.y = maxv;

        bgfx::setVertexBuffer(0, &vb);
    }
}

// ***********************************************************************

void ShaderFreeCallback(void* ptr, void* userData) {
    g_Allocator.Free(ptr);
}

// ***********************************************************************

bgfx::ShaderHandle LoadShader(const char* path) {
    SDL_RWops* pFileRead = SDL_RWFromFile(path, "rb");
    u64 size = SDL_RWsize(pFileRead);
    char* pData = (char*)g_Allocator.Allocate(size * sizeof(char));
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);

    return bgfx::createShader(bgfx::makeRef(pData, (u32)size, ShaderFreeCallback));
}

// ***********************************************************************

void GraphicsChip::Init() {
    layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .end();

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/bin/core3d.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/bin/core3d.fbin");
        programBase3D = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/bin/core3d.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/bin/core3d_textured.fbin");
        programTexturing3D = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/bin/core2d.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/bin/core2d.fbin");
        programBase2D = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/bin/core2d.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/bin/core2d_textured.fbin");
        programTexturing2D = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/bin/fullscreen.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/bin/fullscreen.fbin");
        fullscreenTexProgram = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/bin/fullscreen.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/bin/crt.fbin");

        crtProgram = bgfx::createProgram(vsShader, fsShader, true);
    }

    for (usize i = 0; i < 3; i++) {
        matrixStates[i] = Matrixf::Identity();
    }

    const u64 tsFlags = 0
                             | BGFX_TEXTURE_RT
                             | BGFX_SAMPLER_POINT
                             | BGFX_SAMPLER_U_CLAMP
                             | BGFX_SAMPLER_V_CLAMP;

    // Frame buffers
    bgfx::TextureHandle gbuffer3dTex[] = {
        bgfx::createTexture2D(u16(targetResolution.x), u16(targetResolution.y), false, 1, bgfx::TextureFormat::RGBA32F, tsFlags),
        bgfx::createTexture2D(u16(targetResolution.x), u16(targetResolution.y), false, 1, bgfx::TextureFormat::D32F, tsFlags),
    };
    frameBuffer3D = bgfx::createFrameBuffer(BX_COUNTOF(gbuffer3dTex), gbuffer3dTex, true);
    bgfx::TextureHandle gbuffer2dTex[] = {
        bgfx::createTexture2D(u16(targetResolution.x), u16(targetResolution.y), false, 1, bgfx::TextureFormat::RGBA32F, tsFlags),
        bgfx::createTexture2D(u16(targetResolution.x), u16(targetResolution.y), false, 1, bgfx::TextureFormat::D32F, tsFlags),
    };
    frameBuffer2D = bgfx::createFrameBuffer(BX_COUNTOF(gbuffer2dTex), gbuffer2dTex, true);
    bgfx::TextureHandle gbufferCompositeTex[] = {
        bgfx::createTexture2D(u16(targetResolution.x), u16(targetResolution.y), false, 1, bgfx::TextureFormat::RGBA32F, tsFlags),
        bgfx::createTexture2D(u16(targetResolution.x), u16(targetResolution.y), false, 1, bgfx::TextureFormat::D32F, tsFlags),
    };
    frameBufferComposite = bgfx::createFrameBuffer(BX_COUNTOF(gbufferCompositeTex), gbufferCompositeTex, true);

    defaultFont = Font("assets/Roboto-Bold.ttf");

    frameBufferSampler = bgfx::createUniform("fullscreenFrameSampler", bgfx::UniformType::Sampler);
    colorTextureSampler = bgfx::createUniform("colorTextureSampler", bgfx::UniformType::Sampler);
    targetResolutionUniform = bgfx::createUniform("u_targetResolution", bgfx::UniformType::Vec4);
    lightingStateUniform = bgfx::createUniform("u_lightingEnabled", bgfx::UniformType::Vec4);
    lightDirectionUniform = bgfx::createUniform("u_lightDirection", bgfx::UniformType::Vec4, MAX_LIGHTS);
    lightColorUniform = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4, MAX_LIGHTS);
    lightAmbientUniform = bgfx::createUniform("u_lightAmbient", bgfx::UniformType::Vec4);
    fogDepthsUniform = bgfx::createUniform("u_fogDepths", bgfx::UniformType::Vec4);
    fogColorUniform = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);
    crtDataUniform = bgfx::createUniform("u_crtData", bgfx::UniformType::Vec4);
}

// ***********************************************************************

void GraphicsChip::DrawFrame(f32 w, f32 h) {
    // 3D and 2D framebuffers are drawn to the composite frame buffer, which is then post processed and drawn to the actual back buffer
    bgfx::setViewClear(compositeView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x400040ff, 1.0f, 0);
    bgfx::setViewRect(compositeView, 0, 0, (u16)w, (u16)h);
    bgfx::setViewMode(compositeView, bgfx::ViewMode::Sequential);
    bgfx::setViewFrameBuffer(compositeView, frameBufferComposite);

    Matrixf ortho = Matrixf::Orthographic(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 100.0f);

    // Draw 3D view
    bgfx::setState(BGFX_STATE_WRITE_RGB);
    bgfx::setViewTransform(compositeView, NULL, &ortho);
    bgfx::setTexture(0, frameBufferSampler, bgfx::getTexture(frameBuffer3D));
    FullScreenQuad(w, h, 0.0f, true, 0.0f);
    bgfx::submit(compositeView, fullscreenTexProgram);

    // Draw 2d View on top
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_WRITE_A);
    bgfx::setViewTransform(compositeView, NULL, &ortho);
    bgfx::setTexture(0, frameBufferSampler, bgfx::getTexture(frameBuffer2D));
    FullScreenQuad(w, h, 0.0f, true, 0.0f);
    bgfx::submit(compositeView, fullscreenTexProgram);

    // Now draw composite view onto the real back buffer with the post process shaders
    bgfx::setViewClear(realWindowView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x400040ff, 1.0f, 0);
    bgfx::setViewRect(realWindowView, 0, 0, (u16)w, (u16)h);
    bgfx::setViewMode(realWindowView, bgfx::ViewMode::Sequential);

    Vec4f crtData = Vec4f(w, h, f32(SDL_GetTicks()) / 1000.0f, 0.0f);
    bgfx::setUniform(crtDataUniform, &crtData);

    bgfx::setState(BGFX_STATE_WRITE_RGB);
    bgfx::setViewTransform(realWindowView, NULL, &ortho);
    bgfx::setTexture(0, frameBufferSampler, bgfx::getTexture(frameBufferComposite));
    FullScreenQuad(w, h, 0.0f, true, 0.0f);
    bgfx::submit(realWindowView, crtProgram);

    for (usize i = 0; i < (int)EMatrixMode::Count; i++) {
        matrixStates[i] = Matrixf::Identity();
    }
}

// ***********************************************************************

void GraphicsChip::BeginObject2D(EPrimitiveType type) {
    typeState = type;
    mode = ERenderMode::Mode2D;
}

// ***********************************************************************

void GraphicsChip::EndObject2D() {
    u64 state = 0
                     | BGFX_STATE_WRITE_RGB
                     | BGFX_STATE_WRITE_A
                     | BGFX_STATE_WRITE_Z
                     | BGFX_STATE_DEPTH_TEST_LESS
                     | BGFX_STATE_BLEND_ALPHA;

    switch (typeState) {
        case EPrimitiveType::Points:
            state |= BGFX_STATE_PT_POINTS;
            break;
        case EPrimitiveType::Lines:
            state |= BGFX_STATE_PT_LINES;
            break;
        case EPrimitiveType::LineStrip:
            state |= BGFX_STATE_PT_LINESTRIP;
            break;
        case EPrimitiveType::Triangles:  // default (has no define)
            break;
    }

    u32 clear = (u8(clearColor.w * 255) << 0) + (u8(clearColor.z * 255) << 8) + (u8(clearColor.y * 255) << 16) + (u8(clearColor.x * 255) << 24);
    bgfx::setViewClear(scene2DView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clear, 1.0f, 0);
    bgfx::setViewRect(scene2DView, 0, 0, u16(targetResolution.x), u16(targetResolution.y));
    bgfx::setViewFrameBuffer(scene2DView, frameBuffer2D);

    // TODO: Consider allowing projection and view matrices on the 2D layer
    Matrixf ortho = Matrixf::Orthographic(0.0f, targetResolution.x, 0.0f, targetResolution.y, -100.0f, 100.0f);
    bgfx::setViewTransform(scene2DView, NULL, &ortho);

    bgfx::setTransform(&matrixStates[(usize)EMatrixMode::Model]);
    bgfx::setState(state);

    // create vertex buffer
    bgfx::TransientVertexBuffer vertexBuffer;
    u32 numVertices = (u32)vertexState.count;
    if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, layout))
        return;  // TODO: Log some error
    bgfx::allocTransientVertexBuffer(&vertexBuffer, numVertices, layout);
    VertexData* verts = (VertexData*)vertexBuffer.data;
    bx::memCopy(verts, vertexState.pData, numVertices * sizeof(VertexData));
    bgfx::setVertexBuffer(0, &vertexBuffer);

    if (pTextureState && bgfx::isValid(pTextureState->handle)) {
        bgfx::setTexture(0, colorTextureSampler, pTextureState->handle);
        bgfx::submit(scene2DView, programTexturing2D);
    } else {
        bgfx::submit(scene2DView, programBase2D);  // TODO: a program that supports no textures
    }

    vertexState.Free();  // TODO: See about using reset here instead
    vertexColorState = Vec4f(1.0f);
    vertexTexCoordState = Vec2f();
    vertexNormalState = Vec3f();
    mode = ERenderMode::None;
}

// ***********************************************************************

void GraphicsChip::BeginObject3D(EPrimitiveType type) {
    // Set draw topology type
    typeState = type;
    mode = ERenderMode::Mode3D;
}

// ***********************************************************************

void GraphicsChip::EndObject3D() {
    if (mode == ERenderMode::None)  // TODO Call errors when this is incorrect
        return;

    u64 state = 0
                     | BGFX_STATE_WRITE_RGB
                     | BGFX_STATE_WRITE_Z
                     | BGFX_STATE_DEPTH_TEST_LESS
                     | BGFX_STATE_BLEND_ALPHA;

    bgfx::TransientVertexBuffer vertexBuffer;
    bgfx::TransientIndexBuffer indexBuffer;
    i32 numIndices;

    vertexBuffer.data = nullptr;

    switch (typeState) {
        case EPrimitiveType::Points:
            state |= BGFX_STATE_PT_POINTS;
            break;
        case EPrimitiveType::Lines:
            state |= BGFX_STATE_PT_LINES;
            break;
        case EPrimitiveType::LineStrip:
            state |= BGFX_STATE_PT_LINESTRIP;
            break;
        case EPrimitiveType::Triangles: {
            if (normalsModeState == ENormalsMode::Flat) {
                for (size i = 0; i < vertexState.count; i += 3) {
                    Vec3f v1 = vertexState[i + 1].pos - vertexState[i].pos;
                    Vec3f v2 = vertexState[i + 2].pos - vertexState[i].pos;
                    Vec3f faceNormal = Vec3f::Cross(v1, v2).GetNormalized();

                    vertexState[i].norm = faceNormal;
                    vertexState[i + 1].norm = faceNormal;
                    vertexState[i + 2].norm = faceNormal;
                }

                // create vertex buffer
                u32 numVertices = (u32)vertexState.count;
                if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, layout))
                    return;
                bgfx::allocTransientVertexBuffer(&vertexBuffer, numVertices, layout);
                VertexData* verts = (VertexData*)vertexBuffer.data;
                bx::memCopy(verts, vertexState.pData, numVertices * sizeof(VertexData));
            } else if (normalsModeState == ENormalsMode::Smooth) {
                // Convert to indexed list, loop through, saving verts i32o vector, each new one you search for in vector, if you find it, save index in index list.
                ResizableArray<VertexData> uniqueVerts;
                defer(uniqueVerts.Free());
                ResizableArray<u16> indices;
                defer(indices.Free());
                for (size i = 0; i < vertexState.count; i++) {
                    VertexData* pVertData = uniqueVerts.Find(vertexState[i]);
                    if (pVertData == uniqueVerts.end()) {
                        // New vertex
                        uniqueVerts.PushBack(vertexState[i]);
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

                // create vertex buffer
                u32 numVertices = (u32)uniqueVerts.count;
                if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, layout))
                    return;
                bgfx::allocTransientVertexBuffer(&vertexBuffer, numVertices, layout);
                VertexData* pDstVerts = (VertexData*)vertexBuffer.data;
                bx::memCopy(pDstVerts, uniqueVerts.pData, numVertices * sizeof(VertexData));

                // Create index buffer
                numIndices = (u32)indices.count;
                if (numIndices != bgfx::getAvailTransientIndexBuffer(numIndices))
                    return;
                bgfx::allocTransientIndexBuffer(&indexBuffer, numIndices);
                u16* pDstIndices = (u16*)indexBuffer.data;
                bx::memCopy(pDstIndices, indices.pData, numIndices * sizeof(u16));
            }
        }
        default:
            break;
    }

    if (vertexBuffer.data == nullptr) {
        // create vertex buffer
        u32 numVertices = (u32)vertexState.count;
        if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, layout))
            return;
        bgfx::allocTransientVertexBuffer(&vertexBuffer, numVertices, layout);
        VertexData* verts = (VertexData*)vertexBuffer.data;
        bx::memCopy(verts, vertexState.pData, numVertices * sizeof(VertexData));
    }

    // Submit draw call

    u32 clear = (u8(clearColor.w * 255) << 0) + (u8(clearColor.z * 255) << 8) + (u8(clearColor.y * 255) << 16) + (u8(clearColor.x * 255) << 24);
    bgfx::setViewClear(scene3DView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clear, 1.0f, 0);
    bgfx::setViewRect(scene3DView, 0, 0, u16(targetResolution.x), u16(targetResolution.y));
    bgfx::setViewFrameBuffer(scene3DView, frameBuffer3D);

    bgfx::setViewTransform(scene3DView, &matrixStates[(usize)EMatrixMode::View], &matrixStates[(usize)EMatrixMode::Projection]);
    bgfx::setTransform(&matrixStates[(usize)EMatrixMode::Model]);
    bgfx::setState(state);
    bgfx::setVertexBuffer(0, &vertexBuffer);
    if (normalsModeState == ENormalsMode::Smooth)
        bgfx::setIndexBuffer(&indexBuffer, 0, numIndices);

    Vec4f targetRes = Vec4f(targetResolution.x, targetResolution.y, 0.f, 0.f);
    bgfx::setUniform(targetResolutionUniform, &targetRes);
    Vec4f lightMode = Vec4f((f32)lightingState);
    bgfx::setUniform(lightingStateUniform, &lightMode);
    bgfx::setUniform(lightDirectionUniform, lightDirectionsStates);
    bgfx::setUniform(lightColorUniform, lightColorStates);
    Vec4f ambient = Vec4f::Embed3D(lightAmbientState);
    bgfx::setUniform(lightAmbientUniform, &ambient);
    Vec4f fogDepths4D = Vec4f::Embed2D(fogDepths);
	fogDepths4D.z = f32(fogState);
	bgfx::setUniform(fogDepthsUniform, &fogDepths4D);
    Vec4f fogColor4D = Vec4f::Embed3D(fogColor);
	bgfx::setUniform(fogColorUniform, &fogColor4D);

    if (pTextureState && bgfx::isValid(pTextureState->handle)) {
        bgfx::setTexture(0, colorTextureSampler, pTextureState->handle);
        bgfx::submit(scene3DView, programTexturing3D);
    } else {
        bgfx::submit(scene3DView, programBase3D);
    }

    vertexState.Free();
    vertexColorState = Vec4f(1.0f);
    vertexTexCoordState = Vec2f();
    vertexNormalState = Vec3f();
    mode = ERenderMode::None;
}

// ***********************************************************************

void GraphicsChip::Vertex(Vec3f vec) {
    vertexState.PushBack({ vec, vertexColorState, vertexTexCoordState, vertexNormalState });
}

// ***********************************************************************

void GraphicsChip::Vertex(Vec2f vec) {
    vertexState.PushBack({ Vec3f::Embed2D(vec), vertexColorState, vertexTexCoordState, Vec3f() });
}

// ***********************************************************************

void GraphicsChip::Color(Vec4f col) {
    vertexColorState = col;
}

// ***********************************************************************

void GraphicsChip::TexCoord(Vec2f tex) {
    vertexTexCoordState = tex;
}

// ***********************************************************************

void GraphicsChip::Normal(Vec3f norm) {
    vertexNormalState = norm;
}

// ***********************************************************************

void GraphicsChip::SetClearColor(Vec4f color) {
    clearColor = color;
}

// ***********************************************************************

void GraphicsChip::MatrixMode(EMatrixMode mode) {
    matrixModeState = mode;
}

// ***********************************************************************

void GraphicsChip::Perspective(f32 screenWidth, f32 screenHeight, f32 nearPlane, f32 farPlane, f32 fov) {
    matrixStates[(usize)matrixModeState] *= Matrixf::Perspective(screenWidth, screenHeight, nearPlane, farPlane, fov);
}

// ***********************************************************************

void GraphicsChip::Translate(Vec3f translation) {
    matrixStates[(usize)matrixModeState] *= Matrixf::MakeTranslation(translation);
}

// ***********************************************************************

void GraphicsChip::Rotate(Vec3f rotation) {
    matrixStates[(usize)matrixModeState] *= Matrixf::MakeRotation(rotation);
}

// ***********************************************************************

void GraphicsChip::Scale(Vec3f scaling) {
    matrixStates[(usize)matrixModeState] *= Matrixf::MakeScale(scaling);
}

// ***********************************************************************

void GraphicsChip::Identity() {
    matrixStates[(usize)matrixModeState] = Matrixf::Identity();
}

// ***********************************************************************

void GraphicsChip::BindTexture(Image* pImage) {
    if (pTextureState != nullptr)
        UnbindTexture();

    // Save as current texture state for binding in endObject
    pTextureState = pImage;
    pTextureState->Retain();
}

// ***********************************************************************

void GraphicsChip::UnbindTexture() {
    pTextureState->Release();
    pTextureState = nullptr;
}

// ***********************************************************************

void GraphicsChip::NormalsMode(ENormalsMode mode) {
    normalsModeState = mode;
}

// ***********************************************************************

void GraphicsChip::EnableLighting(bool enabled) {
    lightingState = enabled;
}

// ***********************************************************************

void GraphicsChip::Light(int id, Vec3f direction, Vec3f color) {
    if (id > 2)
        return;

    lightDirectionsStates[id] = direction;
    lightColorStates[id] = color;
}

// ***********************************************************************

void GraphicsChip::Ambient(Vec3f color) {
    lightAmbientState = color;
}

// ***********************************************************************

void GraphicsChip::EnableFog(bool enabled) {
    fogState = enabled;
}

// ***********************************************************************

void GraphicsChip::SetFogStart(f32 start) {
    fogDepths.x = start;
}

// ***********************************************************************

void GraphicsChip::SetFogEnd(f32 end) {
    fogDepths.y = end;
}

// ***********************************************************************

void GraphicsChip::SetFogColor(Vec3f color) {
    fogColor = color;
}

/*
********************************
*   EXTENDED GRAPHICS LIBRARY
********************************
*/

// ***********************************************************************

void GraphicsChip::DrawSprite(Image* pImage, Vec2f position) {
    DrawSpriteRect(pImage, Vec4f(0.0f, 0.0f, 1.0f, 1.0f), position);
}

// ***********************************************************************

void GraphicsChip::DrawSpriteRect(Image* pImage, Vec4f rect, Vec2f position) {
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

void GraphicsChip::DrawText(const char* text, Vec2f position, f32 size) {
    DrawTextEx(text, position, Vec4f(1.0f, 1.0f, 1.0f, 1.0f), &defaultFont, size);
}

// ***********************************************************************

void GraphicsChip::DrawTextEx(const char* text, Vec2f position, Vec4f color, Font* pFont, f32 fontSize) {
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

void GraphicsChip::DrawPixel(Vec2f position, Vec4f color) {
    BeginObject2D(EPrimitiveType::Points);
    Color(color);
    Vertex(position);
    EndObject2D();
}

// ***********************************************************************

void GraphicsChip::DrawLine(Vec2f start, Vec2f end, Vec4f color) {
    BeginObject2D(EPrimitiveType::Lines);
    Color(color);
    Vertex(start);
    Vertex(end);
    EndObject2D();
}

// ***********************************************************************

void GraphicsChip::DrawRectangle(Vec2f bottomLeft, Vec2f topRight, Vec4f color) {
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

void GraphicsChip::DrawRectangleOutline(Vec2f bottomLeft, Vec2f topRight, Vec4f color) {
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

void GraphicsChip::DrawCircle(Vec2f center, f32 radius, Vec4f color) {
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

void GraphicsChip::DrawCircleOutline(Vec2f center, f32 radius, Vec4f color) {
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
