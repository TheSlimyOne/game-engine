// FullscreenQuad.cpp

#include "FullscreenQuad.h"

bgfx::VertexLayout FullscreenVertex::layout;

void FullscreenVertex::initLayout()
{
    static bool initialized = false;
    if (initialized)
        return;

    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    initialized = true;
}


void FullscreenQuad::init()
{
    FullscreenVertex::initLayout();

    // Positions in clip space [-1,1], UVs [0,1]
    FullscreenVertex vertices[4] =
    {
        { -1.0f,  1.0f, 0.5f, 0.0f, 0.0f },
        {  1.0f,  1.0f, 0.5f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 0.5f, 0.0f, 1.0f },
        {  1.0f, -1.0f, 0.5f, 1.0f, 1.0f },
    };

    const uint16_t indices[6] = { 0, 1, 2, 1, 3, 2 };

    // FIX 1: Use bgfx::copy instead of bgfx::makeRef
    m_vbh = bgfx::createVertexBuffer(
        bgfx::copy(vertices, sizeof(vertices)),
        FullscreenVertex::layout
    );

    // FIX 2: Use bgfx::copy for indices too
    m_ibh = bgfx::createIndexBuffer(
        bgfx::copy(indices, sizeof(indices))
    );
}

void FullscreenQuad::shutdown()
{
    if (bgfx::isValid(m_vbh)) bgfx::destroy(m_vbh);
    if (bgfx::isValid(m_ibh)) bgfx::destroy(m_ibh);
    m_vbh = BGFX_INVALID_HANDLE;
    m_ibh = BGFX_INVALID_HANDLE;
}

void FullscreenQuad::draw(bgfx::ViewId /*viewId*/)
{
    bgfx::setVertexBuffer(0, m_vbh);
    bgfx::setIndexBuffer(m_ibh);
}
