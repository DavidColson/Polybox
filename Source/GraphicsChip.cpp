#include "GraphicsChip.h"

#include <bx/bx.h>
#include <shaderc/shaderc.h>


// ***********************************************************************

void GraphicsChip::ScreenSpaceQuad(float _textureWidth, float _textureHeight, float _texelHalf, bool _originBottomLeft, float _width, float _height)
{
	if (3 == bgfx::getAvailTransientVertexBuffer(3, m_layout) )
	{
		bgfx::TransientVertexBuffer vb;
		bgfx::allocTransientVertexBuffer(&vb, 3, m_layout);
		VertexData* vertex = (VertexData*)vb.data;

		const float minx = -_width;
		const float maxx =  _width;
		const float miny = 0.0f;
		const float maxy = _height*2.0f;

		const float texelHalfW = _texelHalf/_textureWidth;
		const float texelHalfH = _texelHalf/_textureHeight;
		const float minu = -1.0f + texelHalfW;
		const float maxu =  1.0f + texelHalfH;

		const float zz = 0.0f;

		float minv = texelHalfH;
		float maxv = 2.0f + texelHalfH;

		if (_originBottomLeft)
		{
			float temp = minv;
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

void GraphicsChip::Init()
{
    m_layout
    .begin()
    .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0,  4, bgfx::AttribType::Float)
    .add(bgfx::Attrib::TexCoord0,  2, bgfx::AttribType::Float)
    .end();

    {
        const bgfx::Memory* pFsShaderMem = nullptr;
        pFsShaderMem = shaderc::compileShader(shaderc::ST_FRAGMENT, "Shaders/core.fs", "", "Shaders/varying.def.sc");
        bgfx::ShaderHandle fsShader = bgfx::createShader(pFsShaderMem);

        const bgfx::Memory* pVsShaderMem = nullptr;
        pVsShaderMem = shaderc::compileShader(shaderc::ST_VERTEX, "Shaders/core.vs", "", "Shaders/varying.def.sc");
        bgfx::ShaderHandle vsShader = bgfx::createShader(pVsShaderMem);

        m_coreProgram = bgfx::createProgram(vsShader, fsShader, true);
    }

    {
        const bgfx::Memory* pFsShaderMem = nullptr;
        pFsShaderMem = shaderc::compileShader(shaderc::ST_FRAGMENT, "Shaders/fullscreen.fs", "", "Shaders/varying.def.sc");
        bgfx::ShaderHandle fsShader = bgfx::createShader(pFsShaderMem);

        const bgfx::Memory* pVsShaderMem = nullptr;
        pVsShaderMem = shaderc::compileShader(shaderc::ST_VERTEX, "Shaders/fullscreen.vs", "", "Shaders/varying.def.sc");
        bgfx::ShaderHandle vsShader = bgfx::createShader(pVsShaderMem);

        m_fullscreenTexProgram = bgfx::createProgram(vsShader, fsShader, true);
    }

    for (size_t i = 0; i < 3; i++)
    {
        m_matrixStates[i] = Matrixf::Identity();
    }
    
    const uint64_t tsFlags = 0
			| BGFX_TEXTURE_RT
			| BGFX_SAMPLER_POINT
			| BGFX_SAMPLER_U_CLAMP
			| BGFX_SAMPLER_V_CLAMP
			;

    bgfx::TextureHandle gbufferTex[] =
					{
						bgfx::createTexture2D(320, 240, false, 1, bgfx::TextureFormat::RGBA32F, tsFlags),
						bgfx::createTexture2D(320, 240, false, 1, bgfx::TextureFormat::D32F,tsFlags),
					};

    m_frameBuffer = bgfx::createFrameBuffer(BX_COUNTOF(gbufferTex), gbufferTex, true);
    m_frameBufferSampler = bgfx::createUniform("m_textureSampler", bgfx::UniformType::Sampler);
}

// ***********************************************************************

void GraphicsChip::DrawFrame(float w, float h)
{
    bgfx::setViewClear(m_realWindowView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x400040ff, 1.0f, 0);
    bgfx::setViewRect(m_realWindowView, 0, 0, (uint16_t)w, (uint16_t)h);

    Matrixf ortho = Matrixf::Orthographic(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 100.0f);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A );
	bgfx::setViewTransform(m_realWindowView, NULL, &ortho);
    bgfx::setTexture(0, m_frameBufferSampler, bgfx::getTexture(m_frameBuffer));
    ScreenSpaceQuad(w, h, 0.0f, true);
    bgfx::submit(m_realWindowView, m_fullscreenTexProgram);
}

// ***********************************************************************

void GraphicsChip::BeginObject(PrimitiveType type)
{
    // Set draw topology type
    m_typeState = type;
}

// ***********************************************************************

void GraphicsChip::EndObject()
{
    uint64_t state = 0
					| BGFX_STATE_WRITE_RGB
					| BGFX_STATE_WRITE_A
					| BGFX_STATE_WRITE_Z
					| BGFX_STATE_DEPTH_TEST_LESS;

    switch (m_typeState)
    {
    case PrimitiveType::Points:
        state |= BGFX_STATE_PT_POINTS;
        break;
    case PrimitiveType::Lines:
        state |= BGFX_STATE_PT_LINES;
        break;
    case PrimitiveType::LineStrip:
        state |= BGFX_STATE_PT_LINESTRIP;
        break;
    case PrimitiveType::TriangleStrip:
        state |= BGFX_STATE_PT_TRISTRIP;
        break;
    case PrimitiveType::Triangles:
    default:
        break;
    }
    

    // create bhfx transient buffer
    uint32_t numVertices = (uint32_t)m_vertexState.size();

    if (numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, m_layout) )
    {
        // not enough space in transient buffer just quit drawing the rest...
        return;
    }

    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_layout);

    Vec3f* verts = (Vec3f*)tvb.data;
    bx::memCopy(verts, m_vertexState.data(), numVertices * sizeof(VertexData) );

    // Submit draw call
    bgfx::setViewClear(m_virtualWindowView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x404040ff, 1.0f, 0);
    bgfx::setViewRect(m_virtualWindowView, 0, 0, 320, 240);
    bgfx::setViewFrameBuffer(m_virtualWindowView, m_frameBuffer);

    bgfx::setViewTransform(m_virtualWindowView, &m_matrixStates[(size_t)MatrixMode::View], &m_matrixStates[(size_t)MatrixMode::Projection]);
    bgfx::setTransform(&m_matrixStates[(size_t)MatrixMode::Model]);
	bgfx::setState(state);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::submit(m_virtualWindowView, m_coreProgram);

    m_vertexState.clear();
}

// ***********************************************************************

void GraphicsChip::Vertex(Vec3f vec)
{
    m_vertexState.push_back({vec, m_vertexColorState});
}

// ***********************************************************************

void GraphicsChip::Color(Vec4f col)
{
    m_vertexColorState = col;
}

// ***********************************************************************

void GraphicsChip::SetMatrixMode(MatrixMode mode)
{
    m_matrixModeState = mode;
}

// ***********************************************************************

void GraphicsChip::Perspective(float screenWidth, float screenHeight, float nearPlane, float farPlane, float fov)
{
    m_matrixStates[(size_t)m_matrixModeState] *= Matrixf::Perspective(screenWidth, screenHeight, nearPlane, farPlane, fov);
}

// ***********************************************************************

void GraphicsChip::Translate(Vec3f translation)
{
    m_matrixStates[(size_t)m_matrixModeState] *= Matrixf::MakeTranslation(translation);
}

// ***********************************************************************

void GraphicsChip::Rotate(Vec3f rotation)
{
    m_matrixStates[(size_t)m_matrixModeState] *= Matrixf::MakeRotation(rotation);
}

// ***********************************************************************

void GraphicsChip::Scale(Vec3f scaling)
{
    m_matrixStates[(size_t)m_matrixModeState] *= Matrixf::MakeScale(scaling);
}

// ***********************************************************************

void GraphicsChip::Identity()
{
    m_matrixStates[(size_t)m_matrixModeState] = Matrixf::Identity();
}