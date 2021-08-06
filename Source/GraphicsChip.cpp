#include "GraphicsChip.h"

#include <bx/bx.h>
#include <shaderc/shaderc.h>


void GraphicsChip::Init()
{
    m_layout
    .begin()
    .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
    .end();

    const bgfx::Memory* pFsShaderMem = nullptr;
    pFsShaderMem = shaderc::compileShader(shaderc::ST_FRAGMENT, "Shaders/cubes.fs", "", "Shaders/varying.def.sc");
    bgfx::ShaderHandle fsShader = bgfx::createShader(pFsShaderMem);

    const bgfx::Memory* pVsShaderMem = nullptr;
    pVsShaderMem = shaderc::compileShader(shaderc::ST_VERTEX, "Shaders/cubes.vs", "", "Shaders/varying.def.sc");
    bgfx::ShaderHandle vsShader = bgfx::createShader(pVsShaderMem);

    m_program = bgfx::createProgram(vsShader, fsShader, true);
}

void GraphicsChip::BeginObject(PrimitiveType type)
{
    // Set draw topology type
    m_typeState = type;
}

void GraphicsChip::EndObject()
{
    uint64_t state = 0
					| BGFX_STATE_WRITE_R
					| BGFX_STATE_WRITE_G
					| BGFX_STATE_WRITE_B
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
    bx::memCopy(verts, m_vertexState.data(), numVertices * sizeof(Vec3f) );

    // Submit draw call
	bgfx::setState(state);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::submit(0, m_program);

    m_vertexState.clear();
}

void GraphicsChip::Vertex(Vec3f vec)
{
    m_vertexState.push_back(vec);
}