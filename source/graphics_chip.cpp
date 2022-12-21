// Copyright 2020-2022 David Colson. All rights reserved.

#include "graphics_chip.h"

#include "image.h"

#include <SDL_rwops.h>
#include <SDL_timer.h>
#include <defer.h>
#include <shaderc/shaderc.h>


// ***********************************************************************

uint64_t StringHash(const char* str) {
    uint64_t offset = 14695981039346656037u;
    uint64_t prime = 1099511628211u;

    uint64_t hash = offset;
    while (*str != 0) {
        hash ^= *str++;
        hash *= prime;
    }
    return hash;
}

// ***********************************************************************

void GraphicsChip::FullScreenQuad(float _textureWidth, float _textureHeight, float _texelHalf, bool _originBottomLeft, float _depth, float _width, float _height) {
    if (3 == bgfx::getAvailTransientVertexBuffer(3, m_layout)) {
        bgfx::TransientVertexBuffer vb;
        bgfx::allocTransientVertexBuffer(&vb, 3, m_layout);
        VertexData* vertex = (VertexData*)vb.data;

        const float minx = -_width;
        const float maxx = _width;
        const float miny = 0.0f;
        const float maxy = _height * 2.0f;

        const float texelHalfW = _texelHalf / _textureWidth;
        const float texelHalfH = _texelHalf / _textureHeight;
        const float minu = -1.0f + texelHalfW;
        const float maxu = 1.0f + texelHalfH;

        const float zz = _depth;

        float minv = texelHalfH;
        float maxv = 2.0f + texelHalfH;

        if (_originBottomLeft) {
            float temp = minv;
            minv = maxv;
            maxv = temp;

            minv -= 1.0f;
            maxv -= 1.0f;
        }

        vertex[0].m_pos.x = minx;
        vertex[0].m_pos.y = miny;
        vertex[0].m_pos.z = zz;
        vertex[0].m_tex.x = minu;
        vertex[0].m_tex.y = minv;

        vertex[1].m_pos.x = maxx;
        vertex[1].m_pos.y = miny;
        vertex[1].m_pos.z = zz;
        vertex[1].m_tex.x = maxu;
        vertex[1].m_tex.y = minv;

        vertex[2].m_pos.x = maxx;
        vertex[2].m_pos.y = maxy;
        vertex[2].m_pos.z = zz;
        vertex[2].m_tex.x = maxu;
        vertex[2].m_tex.y = maxv;

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
    uint64_t size = SDL_RWsize(pFileRead);
    char* pData = (char*)g_Allocator.Allocate(size * sizeof(char));
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);

    return bgfx::createShader(bgfx::makeRef(pData, (uint32_t)size, ShaderFreeCallback));
}

// ***********************************************************************

void GraphicsChip::Init() {
    m_layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .end();

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/Bin/core3d.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/Bin/core3d.fbin");
        m_programBase3D = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/Bin/core3d.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/Bin/core3d_textured.fbin");
        m_programTexturing3D = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/Bin/core2d.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/Bin/core2d.fbin");
        m_programBase2D = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/Bin/core2d.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/Bin/core2d_textured.fbin");
        m_programTexturing2D = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/Bin/fullscreen.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/Bin/fullscreen.fbin");
        m_fullscreenTexProgram = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        bgfx::ShaderHandle vsShader = LoadShader("shaders/Bin/fullscreen.vbin");
        bgfx::ShaderHandle fsShader = LoadShader("shaders/Bin/crt.fbin");
        m_crtProgram = bgfx::createProgram(vsShader, fsShader, true);
    }

    for (size_t i = 0; i < 3; i++) {
        m_matrixStates[i] = Matrixf::Identity();
    }

    const uint64_t tsFlags = 0
                             | BGFX_TEXTURE_RT
                             | BGFX_SAMPLER_POINT
                             | BGFX_SAMPLER_U_CLAMP
                             | BGFX_SAMPLER_V_CLAMP;

    // Frame buffers
    bgfx::TextureHandle gbuffer3dTex[] = {
        bgfx::createTexture2D(uint16_t(m_targetResolution.x), uint16_t(m_targetResolution.y), false, 1, bgfx::TextureFormat::RGBA32F, tsFlags),
        bgfx::createTexture2D(uint16_t(m_targetResolution.x), uint16_t(m_targetResolution.y), false, 1, bgfx::TextureFormat::D32F, tsFlags),
    };
    m_frameBuffer3D = bgfx::createFrameBuffer(BX_COUNTOF(gbuffer3dTex), gbuffer3dTex, true);
    bgfx::TextureHandle gbuffer2dTex[] = {
        bgfx::createTexture2D(uint16_t(m_targetResolution.x), uint16_t(m_targetResolution.y), false, 1, bgfx::TextureFormat::RGBA32F, tsFlags),
        bgfx::createTexture2D(uint16_t(m_targetResolution.x), uint16_t(m_targetResolution.y), false, 1, bgfx::TextureFormat::D32F, tsFlags),
    };
    m_frameBuffer2D = bgfx::createFrameBuffer(BX_COUNTOF(gbuffer2dTex), gbuffer2dTex, true);
    bgfx::TextureHandle gbufferCompositeTex[] = {
        bgfx::createTexture2D(uint16_t(m_targetResolution.x), uint16_t(m_targetResolution.y), false, 1, bgfx::TextureFormat::RGBA32F, tsFlags),
        bgfx::createTexture2D(uint16_t(m_targetResolution.x), uint16_t(m_targetResolution.y), false, 1, bgfx::TextureFormat::D32F, tsFlags),
    };
    m_frameBufferComposite = bgfx::createFrameBuffer(BX_COUNTOF(gbufferCompositeTex), gbufferCompositeTex, true);

    m_defaultFont = Font("assets/Roboto-Bold.ttf");

    m_frameBufferSampler = bgfx::createUniform("fullscreenFrameSampler", bgfx::UniformType::Sampler);
    m_colorTextureSampler = bgfx::createUniform("colorTextureSampler", bgfx::UniformType::Sampler);
    m_targetResolutionUniform = bgfx::createUniform("u_targetResolution", bgfx::UniformType::Vec4);
    m_lightingStateUniform = bgfx::createUniform("u_lightingEnabled", bgfx::UniformType::Vec4);
    m_lightDirectionUniform = bgfx::createUniform("u_lightDirection", bgfx::UniformType::Vec4, MAX_LIGHTS);
    m_lightColorUniform = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4, MAX_LIGHTS);
    m_lightAmbientUniform = bgfx::createUniform("u_lightAmbient", bgfx::UniformType::Vec4);
    m_fogDepthsUniform = bgfx::createUniform("u_fogDepths", bgfx::UniformType::Vec4);
    m_fogColorUniform = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);
    m_crtDataUniform = bgfx::createUniform("u_crtData", bgfx::UniformType::Vec4);
}

// ***********************************************************************

void GraphicsChip::DrawFrame(float w, float h) {
    // 3D and 2D framebuffers are drawn to the composite frame buffer, which is then post processed and drawn to the actual back buffer
    bgfx::setViewClear(m_compositeView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x400040ff, 1.0f, 0);
    bgfx::setViewRect(m_compositeView, 0, 0, (uint16_t)w, (uint16_t)h);
    bgfx::setViewMode(m_compositeView, bgfx::ViewMode::Sequential);
    bgfx::setViewFrameBuffer(m_compositeView, m_frameBufferComposite);

    Matrixf ortho = Matrixf::Orthographic(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 100.0f);

    // Draw 3D view
    bgfx::setState(BGFX_STATE_WRITE_RGB);
    bgfx::setViewTransform(m_compositeView, NULL, &ortho);
    bgfx::setTexture(0, m_frameBufferSampler, bgfx::getTexture(m_frameBuffer3D));
    FullScreenQuad(w, h, 0.0f, true, 0.0f);
    bgfx::submit(m_compositeView, m_fullscreenTexProgram);

    // Draw 2d View on top
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_WRITE_A);
    bgfx::setViewTransform(m_compositeView, NULL, &ortho);
    bgfx::setTexture(0, m_frameBufferSampler, bgfx::getTexture(m_frameBuffer2D));
    FullScreenQuad(w, h, 0.0f, true, 0.0f);
    bgfx::submit(m_compositeView, m_fullscreenTexProgram);

    // Now draw composite view onto the real back buffer with the post process shaders
    bgfx::setViewClear(m_realWindowView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x400040ff, 1.0f, 0);
    bgfx::setViewRect(m_realWindowView, 0, 0, (uint16_t)w, (uint16_t)h);
    bgfx::setViewMode(m_realWindowView, bgfx::ViewMode::Sequential);

    Vec4f crtData = Vec4f(w, h, float(SDL_GetTicks()) / 1000.0f, 0.0f);
    bgfx::setUniform(m_crtDataUniform, &crtData);

    bgfx::setState(BGFX_STATE_WRITE_RGB);
    bgfx::setViewTransform(m_realWindowView, NULL, &ortho);
    bgfx::setTexture(0, m_frameBufferSampler, bgfx::getTexture(m_frameBufferComposite));
    FullScreenQuad(w, h, 0.0f, true, 0.0f);
    bgfx::submit(m_realWindowView, m_crtProgram);

    for (size_t i = 0; i < (int)EMatrixMode::Count; i++) {
        m_matrixStates[i] = Matrixf::Identity();
    }
}

// ***********************************************************************

void GraphicsChip::BeginObject2D(EPrimitiveType type) {
    m_typeState = type;
    m_mode = ERenderMode::Mode2D;
}

// ***********************************************************************

void GraphicsChip::EndObject2D() {
    uint64_t state = 0
                     | BGFX_STATE_WRITE_RGB
                     | BGFX_STATE_WRITE_A
                     | BGFX_STATE_WRITE_Z
                     | BGFX_STATE_DEPTH_TEST_LESS
                     | BGFX_STATE_BLEND_ALPHA;

    switch (m_typeState) {
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

    uint32_t clear = (uint8_t(m_clearColor.w * 255) << 0) + (uint8_t(m_clearColor.z * 255) << 8) + (uint8_t(m_clearColor.y * 255) << 16) + (uint8_t(m_clearColor.x * 255) << 24);
    bgfx::setViewClear(m_scene2DView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clear, 1.0f, 0);
    bgfx::setViewRect(m_scene2DView, 0, 0, uint16_t(m_targetResolution.x), uint16_t(m_targetResolution.y));
    bgfx::setViewFrameBuffer(m_scene2DView, m_frameBuffer2D);

    // TODO: Consider allowing projection and view matrices on the 2D layer
    Matrixf ortho = Matrixf::Orthographic(0.0f, m_targetResolution.x, 0.0f, m_targetResolution.y, -100.0f, 100.0f);
    bgfx::setViewTransform(m_scene2DView, NULL, &ortho);

    bgfx::setTransform(&m_matrixStates[(size_t)EMatrixMode::Model]);
    bgfx::setState(state);

    // create vertex buffer
    bgfx::TransientVertexBuffer vertexBuffer;
    uint32_t numVertices = (uint32_t)m_vertexState.m_count;
    if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, m_layout))
        return;  // TODO: Log some error
    bgfx::allocTransientVertexBuffer(&vertexBuffer, numVertices, m_layout);
    VertexData* verts = (VertexData*)vertexBuffer.data;
    bx::memCopy(verts, m_vertexState.m_pData, numVertices * sizeof(VertexData));
    bgfx::setVertexBuffer(0, &vertexBuffer);

    if (m_pTextureState && bgfx::isValid(m_pTextureState->m_handle)) {
        bgfx::setTexture(0, m_colorTextureSampler, m_pTextureState->m_handle);
        bgfx::submit(m_scene2DView, m_programTexturing2D);
    } else {
        bgfx::submit(m_scene2DView, m_programBase2D);  // TODO: a program that supports no textures
    }

    m_vertexState.Free();  // TODO: See about using reset here instead
    m_vertexColorState = Vec4f(1.0f);
    m_vertexTexCoordState = Vec2f();
    m_vertexNormalState = Vec3f();
    m_mode = ERenderMode::None;
}

// ***********************************************************************

void GraphicsChip::BeginObject3D(EPrimitiveType type) {
    // Set draw topology type
    m_typeState = type;
    m_mode = ERenderMode::Mode3D;
}

// ***********************************************************************

void GraphicsChip::EndObject3D() {
    if (m_mode == ERenderMode::None)  // TODO Call errors when this is incorrect
        return;

    uint64_t state = 0
                     | BGFX_STATE_WRITE_RGB
                     | BGFX_STATE_WRITE_Z
                     | BGFX_STATE_DEPTH_TEST_LESS
                     | BGFX_STATE_BLEND_ALPHA;

    bgfx::TransientVertexBuffer vertexBuffer;
    bgfx::TransientIndexBuffer indexBuffer;
    int numIndices;

    vertexBuffer.data = nullptr;

    switch (m_typeState) {
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
            if (m_normalsModeState == ENormalsMode::Flat) {
                for (size_t i = 0; i < m_vertexState.m_count; i += 3) {
                    Vec3f v1 = m_vertexState[i + 1].m_pos - m_vertexState[i].m_pos;
                    Vec3f v2 = m_vertexState[i + 2].m_pos - m_vertexState[i].m_pos;
                    Vec3f faceNormal = Vec3f::Cross(v1, v2).GetNormalized();

                    m_vertexState[i].m_norm = faceNormal;
                    m_vertexState[i + 1].m_norm = faceNormal;
                    m_vertexState[i + 2].m_norm = faceNormal;
                }

                // create vertex buffer
                uint32_t numVertices = (uint32_t)m_vertexState.m_count;
                if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, m_layout))
                    return;
                bgfx::allocTransientVertexBuffer(&vertexBuffer, numVertices, m_layout);
                VertexData* verts = (VertexData*)vertexBuffer.data;
                bx::memCopy(verts, m_vertexState.m_pData, numVertices * sizeof(VertexData));
            } else if (m_normalsModeState == ENormalsMode::Smooth) {
                // Convert to indexed list, loop through, saving verts into vector, each new one you search for in vector, if you find it, save index in index list.
                ResizableArray<VertexData> uniqueVerts;
                defer(uniqueVerts.Free());
                ResizableArray<uint16_t> indices;
                defer(indices.Free());
                for (size_t i = 0; i < m_vertexState.m_count; i++) {
                    VertexData* pVertData = uniqueVerts.Find(m_vertexState[i]);
                    if (pVertData == uniqueVerts.end()) {
                        // New vertex
                        uniqueVerts.PushBack(m_vertexState[i]);
                        indices.PushBack((uint16_t)uniqueVerts.m_count - 1);
                    } else {
                        indices.PushBack((uint16_t)uniqueVerts.IndexFromPointer(pVertData));
                    }
                }

                // Then run your flat shading algo on the list of vertices looping through index list. If you have a new normal for a vert, then average with the existing one
                for (size_t i = 0; i < indices.m_count; i += 3) {
                    Vec3f v1 = uniqueVerts[indices[i + 1]].m_pos - uniqueVerts[indices[i]].m_pos;
                    Vec3f v2 = uniqueVerts[indices[i + 2]].m_pos - uniqueVerts[indices[i]].m_pos;
                    Vec3f faceNormal = Vec3f::Cross(v1, v2);

                    uniqueVerts[indices[i]].m_norm += faceNormal;
                    uniqueVerts[indices[i + 1]].m_norm += faceNormal;
                    uniqueVerts[indices[i + 2]].m_norm += faceNormal;
                }

                for (size_t i = 0; i < uniqueVerts.m_count; i++) {
                    uniqueVerts[i].m_norm = uniqueVerts[i].m_norm.GetNormalized();
                }

                // create vertex buffer
                uint32_t numVertices = (uint32_t)uniqueVerts.m_count;
                if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, m_layout))
                    return;
                bgfx::allocTransientVertexBuffer(&vertexBuffer, numVertices, m_layout);
                VertexData* pDstVerts = (VertexData*)vertexBuffer.data;
                bx::memCopy(pDstVerts, uniqueVerts.m_pData, numVertices * sizeof(VertexData));

                // Create index buffer
                numIndices = (uint32_t)indices.m_count;
                if (numIndices != bgfx::getAvailTransientIndexBuffer(numIndices))
                    return;
                bgfx::allocTransientIndexBuffer(&indexBuffer, numIndices);
                uint16_t* pDstIndices = (uint16_t*)indexBuffer.data;
                bx::memCopy(pDstIndices, indices.m_pData, numIndices * sizeof(uint16_t));
            }
        }
        default:
            break;
    }

    if (vertexBuffer.data == nullptr) {
        // create vertex buffer
        uint32_t numVertices = (uint32_t)m_vertexState.m_count;
        if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, m_layout))
            return;
        bgfx::allocTransientVertexBuffer(&vertexBuffer, numVertices, m_layout);
        VertexData* verts = (VertexData*)vertexBuffer.data;
        bx::memCopy(verts, m_vertexState.m_pData, numVertices * sizeof(VertexData));
    }

    // Submit draw call

    uint32_t clear = (uint8_t(m_clearColor.w * 255) << 0) + (uint8_t(m_clearColor.z * 255) << 8) + (uint8_t(m_clearColor.y * 255) << 16) + (uint8_t(m_clearColor.x * 255) << 24);
    bgfx::setViewClear(m_scene3DView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clear, 1.0f, 0);
    bgfx::setViewRect(m_scene3DView, 0, 0, uint16_t(m_targetResolution.x), uint16_t(m_targetResolution.y));
    bgfx::setViewFrameBuffer(m_scene3DView, m_frameBuffer3D);

    bgfx::setViewTransform(m_scene3DView, &m_matrixStates[(size_t)EMatrixMode::View], &m_matrixStates[(size_t)EMatrixMode::Projection]);
    bgfx::setTransform(&m_matrixStates[(size_t)EMatrixMode::Model]);
    bgfx::setState(state);
    bgfx::setVertexBuffer(0, &vertexBuffer);
    if (m_normalsModeState == ENormalsMode::Smooth)
        bgfx::setIndexBuffer(&indexBuffer, 0, numIndices);

    Vec4f targetRes = Vec4f(m_targetResolution.x, m_targetResolution.y, 0.f, 0.f);
    bgfx::setUniform(m_targetResolutionUniform, &targetRes);
    Vec4f lightMode = Vec4f((float)m_lightingState);
    bgfx::setUniform(m_lightingStateUniform, &lightMode);
    bgfx::setUniform(m_lightDirectionUniform, m_lightDirectionsStates);
    bgfx::setUniform(m_lightColorUniform, m_lightColorStates);
    Vec4f ambient = Vec4f::Embed3D(m_lightAmbientState);
    bgfx::setUniform(m_lightAmbientUniform, &ambient);
    Vec4f fogDepths = Vec4f::Embed2D(m_fogDepths);
    fogDepths.z = float(m_fogState);
    bgfx::setUniform(m_fogDepthsUniform, &fogDepths);
    Vec4f fogColor = Vec4f::Embed3D(m_fogColor);
    bgfx::setUniform(m_fogColorUniform, &fogColor);

    if (m_pTextureState && bgfx::isValid(m_pTextureState->m_handle)) {
        bgfx::setTexture(0, m_colorTextureSampler, m_pTextureState->m_handle);
        bgfx::submit(m_scene3DView, m_programTexturing3D);
    } else {
        bgfx::submit(m_scene3DView, m_programBase3D);
    }

    m_vertexState.Free();
    m_vertexColorState = Vec4f(1.0f);
    m_vertexTexCoordState = Vec2f();
    m_vertexNormalState = Vec3f();
    m_mode = ERenderMode::None;
}

// ***********************************************************************

void GraphicsChip::Vertex(Vec3f vec) {
    m_vertexState.PushBack({ vec, m_vertexColorState, m_vertexTexCoordState, m_vertexNormalState });
}

// ***********************************************************************

void GraphicsChip::Vertex(Vec2f vec) {
    m_vertexState.PushBack({ Vec3f::Embed2D(vec), m_vertexColorState, m_vertexTexCoordState, Vec3f() });
}

// ***********************************************************************

void GraphicsChip::Color(Vec4f col) {
    m_vertexColorState = col;
}

// ***********************************************************************

void GraphicsChip::TexCoord(Vec2f tex) {
    m_vertexTexCoordState = tex;
}

// ***********************************************************************

void GraphicsChip::Normal(Vec3f norm) {
    m_vertexNormalState = norm;
}

// ***********************************************************************

void GraphicsChip::SetClearColor(Vec4f color) {
    m_clearColor = color;
}

// ***********************************************************************

void GraphicsChip::MatrixMode(EMatrixMode mode) {
    m_matrixModeState = mode;
}

// ***********************************************************************

void GraphicsChip::Perspective(float screenWidth, float screenHeight, float nearPlane, float farPlane, float fov) {
    m_matrixStates[(size_t)m_matrixModeState] *= Matrixf::Perspective(screenWidth, screenHeight, nearPlane, farPlane, fov);
}

// ***********************************************************************

void GraphicsChip::Translate(Vec3f translation) {
    m_matrixStates[(size_t)m_matrixModeState] *= Matrixf::MakeTranslation(translation);
}

// ***********************************************************************

void GraphicsChip::Rotate(Vec3f rotation) {
    m_matrixStates[(size_t)m_matrixModeState] *= Matrixf::MakeRotation(rotation);
}

// ***********************************************************************

void GraphicsChip::Scale(Vec3f scaling) {
    m_matrixStates[(size_t)m_matrixModeState] *= Matrixf::MakeScale(scaling);
}

// ***********************************************************************

void GraphicsChip::Identity() {
    m_matrixStates[(size_t)m_matrixModeState] = Matrixf::Identity();
}

// ***********************************************************************

void GraphicsChip::BindTexture(Image* pImage) {
    if (m_pTextureState != nullptr)
        UnbindTexture();

    // Save as current texture state for binding in endObject
    m_pTextureState = pImage;
    m_pTextureState->Retain();
}

// ***********************************************************************

void GraphicsChip::UnbindTexture() {
    m_pTextureState->Release();
    m_pTextureState = nullptr;
}

// ***********************************************************************

void GraphicsChip::NormalsMode(ENormalsMode mode) {
    m_normalsModeState = mode;
}

// ***********************************************************************

void GraphicsChip::EnableLighting(bool enabled) {
    m_lightingState = enabled;
}

// ***********************************************************************

void GraphicsChip::Light(int id, Vec3f direction, Vec3f color) {
    if (id > 2)
        return;

    m_lightDirectionsStates[id] = direction;
    m_lightColorStates[id] = color;
}

// ***********************************************************************

void GraphicsChip::Ambient(Vec3f color) {
    m_lightAmbientState = color;
}

// ***********************************************************************

void GraphicsChip::EnableFog(bool enabled) {
    m_fogState = enabled;
}

// ***********************************************************************

void GraphicsChip::SetFogStart(float start) {
    m_fogDepths.x = start;
}

// ***********************************************************************

void GraphicsChip::SetFogEnd(float end) {
    m_fogDepths.y = end;
}

// ***********************************************************************

void GraphicsChip::SetFogColor(Vec3f color) {
    m_fogColor = color;
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
    float w = (float)pImage->m_width * (rect.z - rect.x);
    float h = (float)pImage->m_height * (rect.w - rect.y);

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

void GraphicsChip::DrawText(const char* text, Vec2f position, float size) {
    DrawTextEx(text, position, Vec4f(1.0f, 1.0f, 1.0f, 1.0f), &m_defaultFont, size);
}

// ***********************************************************************

void GraphicsChip::DrawTextEx(const char* text, Vec2f position, Vec4f color, Font* pFont, float size) {
    constexpr float baseSize = 32.0f;

    float textWidth = 0.0f;
    float x = position.x;
    float y = position.y;
    Vec2f scale = Vec2f(size / baseSize, size / baseSize);

    String stringText(text);
    for (size_t i = 0; i < stringText.m_length; i++) {
        char c = *(stringText.m_pData + i);
        Character ch = pFont->m_characters[c];
        textWidth += ch.m_advance * scale.x;
    }

    BindTexture(&pFont->m_fontTexture);
    BeginObject2D(EPrimitiveType::Triangles);
    for (size_t i = 0; i < stringText.m_length; i++) {
        char c = *(stringText.m_pData + i);
        Character ch = pFont->m_characters[c];

        // Center alignment
        // float xpos = (x + ch.bearing.x * scale.x) - textWidth * 0.5f;
        float xpos = (x + ch.m_bearing.x * scale.x);
        float ypos = y - (ch.m_size.y - ch.m_bearing.y) * scale.y;
        float w = (float)ch.m_size.x * scale.x;
        float h = (float)ch.m_size.y * scale.y;

        // 0
        Color(color);
        TexCoord(Vec2f(ch.m_UV0.x, ch.m_UV1.y));
        Vertex(Vec2f(xpos, ypos));

        // 1
        Color(color);
        TexCoord(Vec2f(ch.m_UV1.x, ch.m_UV0.y));
        Vertex(Vec2f(xpos + w, ypos + h));

        // 2
        Color(color);
        TexCoord(Vec2f(ch.m_UV0.x, ch.m_UV0.y));
        Vertex(Vec2f(xpos, ypos + h));

        // 0
        Color(color);
        TexCoord(Vec2f(ch.m_UV0.x, ch.m_UV1.y));
        Vertex(Vec2f(xpos, ypos));

        // 3
        Color(color);
        TexCoord(Vec2f(ch.m_UV1.x, ch.m_UV1.y));
        Vertex(Vec2f(xpos + w, ypos));

        // 1
        Color(color);
        TexCoord(Vec2f(ch.m_UV1.x, ch.m_UV0.y));
        Vertex(Vec2f(xpos + w, ypos + h));

        x += ch.m_advance * scale.x;
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

void GraphicsChip::DrawCircle(Vec2f center, float radius, Vec4f color) {
    BeginObject2D(EPrimitiveType::Triangles);
    constexpr float segments = 24;
    for (size_t i = 0; i < segments; i++) {
        float x1 = (PI2 / segments) * i;
        float x2 = (PI2 / segments) * (i + 1);
        Color(color);
        Vertex(center);
        Vertex(center + Vec2f(sin(x1), cos(x1)) * radius);
        Vertex(center + Vec2f(sin(x2), cos(x2)) * radius);
    }
    EndObject2D();
}

// ***********************************************************************

void GraphicsChip::DrawCircleOutline(Vec2f center, float radius, Vec4f color) {
    BeginObject2D(EPrimitiveType::Lines);
    constexpr float segments = 24;
    for (size_t i = 0; i < segments; i++) {
        float x1 = (PI2 / segments) * i;
        float x2 = (PI2 / segments) * (i + 1);
        Color(color);
        Vertex(center + Vec2f(sin(x1), cos(x1)) * radius);
        Vertex(center + Vec2f(sin(x2), cos(x2)) * radius);
    }
    EndObject2D();
}
