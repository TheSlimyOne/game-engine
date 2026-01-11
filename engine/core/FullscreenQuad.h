// FullscreenQuad.h

#ifndef GAME_FULLSCREENQUAD_H
#define GAME_FULLSCREENQUAD_H

#include <bgfx/bgfx.h>

struct FullscreenVertex
{
    float x, y, z;
    float u, v;

    static bgfx::VertexLayout layout;
    static void initLayout();
};

class FullscreenQuad
{
public:
    FullscreenQuad() = default;
    ~FullscreenQuad() = default;

    void init();
    void shutdown();

    // Just binds VB/IB; you still call bgfx::submit yourself.
    void draw(bgfx::ViewId viewId);

private:
    bgfx::VertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle  m_ibh = BGFX_INVALID_HANDLE;
};

#endif //GAME_FULLSCREENQUAD_H