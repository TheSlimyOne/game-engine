// OutlineEffect.h

#ifndef GAME_OUTLINEEFFECT_H
#define GAME_OUTLINEEFFECT_H

#include "PostProcessEffect.h"
#include <core/FullscreenQuad.h>

class OutlineEffect : public PostProcessEffect
{
public:
    OutlineEffect(FullscreenQuad& quad, uint16_t width, uint16_t height);
    ~OutlineEffect() override;

    void resize(uint16_t width, uint16_t height) override;

    void apply(PostProcessContext& ctx,
               bgfx::TextureHandle srcColor,
               bgfx::FrameBufferHandle dstFb) override;

    // optional tuning:
    void set_outline_color(float r, float g, float b);
    void set_outline_thickness(float pixels);

private:
    FullscreenQuad& m_quad;

    bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_sceneColor = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_maskTex    = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_texelSize  = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_outlineColor = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle u_outlineThickness = BGFX_INVALID_HANDLE;

    float m_outlineColor[4] = {1.0f, 0.6f, 0.1f, 1.0f}; // orange-ish
    float m_thickness = 1.0f; // in pixels
};

#endif //GAME_OUTLINEEFFECT_H