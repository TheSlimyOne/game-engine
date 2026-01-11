// OutlineEffect.cpp

#include "OutlineEffect.h"
#include <shader/ShaderUtils.h>

#include <bgfx/bgfx.h>
#include <iostream>
#include <core/Paths.h>

OutlineEffect::OutlineEffect(FullscreenQuad& quad, uint16_t width, uint16_t height) : m_quad(quad)
{
    (void)width;
    (void)height;

    // Adjust this to your actual path layout
    // e.g. shaders/post/outline/vs_outline.bin, fs_outline.bin
    std::string baseDir = (ROOT / "shaders/post/outline").string();
    std::string vsPath  = baseDir + "/vs_outline.bin";
    std::string fsPath  = baseDir + "/fs_outline.bin";

    m_program = ShaderUtils::load_program(vsPath, fsPath, /*destroyShaders=*/true);

    // CHECK 4: Specific Shader Loading failure
    if (!bgfx::isValid(m_program)) {
        std::cerr << "!!! [OutlineEffect] CRITICAL ERROR: Failed to load shader program !!!" << std::endl;
        std::cerr << "    VS: " << vsPath << std::endl;
        std::cerr << "    FS: " << fsPath << std::endl;
    } else {
        std::cout << "[OutlineEffect] Shader program loaded. Handle: " << m_program.idx << std::endl;
    }

    u_sceneColor        = bgfx::createUniform("s_sceneColor",       bgfx::UniformType::Sampler);
    u_maskTex           = bgfx::createUniform("s_mask",             bgfx::UniformType::Sampler);
    u_texelSize         = bgfx::createUniform("u_texelSize",        bgfx::UniformType::Vec4);
    u_outlineColor      = bgfx::createUniform("u_outlineColor",     bgfx::UniformType::Vec4);
    u_outlineThickness  = bgfx::createUniform("u_outlineThickness", bgfx::UniformType::Vec4);

    // Defaults
    set_outline_color(1.0f, 0.6f, 0.1f); // orange-ish
    set_outline_thickness(1.5f);         // ~1.5 pixels
}

OutlineEffect::~OutlineEffect()
{
    if (bgfx::isValid(m_program))            bgfx::destroy(m_program);
    if (bgfx::isValid(u_sceneColor))         bgfx::destroy(u_sceneColor);
    if (bgfx::isValid(u_maskTex))            bgfx::destroy(u_maskTex);
    if (bgfx::isValid(u_texelSize))          bgfx::destroy(u_texelSize);
    if (bgfx::isValid(u_outlineColor))       bgfx::destroy(u_outlineColor);
    if (bgfx::isValid(u_outlineThickness))   bgfx::destroy(u_outlineThickness);
}

void OutlineEffect::resize(uint16_t width, uint16_t height)
{
    // Nothing to recreate here; we recompute texel size every frame in apply().
    (void)width;
    (void)height;
}

void OutlineEffect::set_outline_color(float r, float g, float b)
{
    m_outlineColor[0] = r;
    m_outlineColor[1] = g;
    m_outlineColor[2] = b;
    m_outlineColor[3] = 1.0f;
}

void OutlineEffect::set_outline_thickness(float pixels)
{
    m_thickness = pixels;
}

void OutlineEffect::apply(PostProcessContext& ctx, bgfx::TextureHandle srcColor, bgfx::FrameBufferHandle dstFb) {
    // We absolutely need a valid source color and program.
    if (!bgfx::isValid(srcColor)) {
        std::cerr << "[OutlineEffect] Apply skipped: Source Texture Invalid." << std::endl;
        return;
    }
    if (!bgfx::isValid(m_program)) {
        std::cerr << "[OutlineEffect] Apply skipped: Program Invalid." << std::endl;
        return;
    }

    const bool hasMask = bgfx::isValid(ctx.selectionMask);

    const bgfx::ViewId viewId = ctx.nextViewId++;

    // dstFb may be BGFX_INVALID_HANDLE => backbuffer
    bgfx::setViewFrameBuffer(viewId, dstFb);
    bgfx::setViewRect(viewId, 0, 0, ctx.width, ctx.height);
    bgfx::setViewClear(viewId, BGFX_CLEAR_NONE, 0, 1.0f, 0);
    bgfx::touch(viewId);

    float texelSize[4] = {
        1.0f / float(ctx.width),
        1.0f / float(ctx.height),
        0.0f, 0.0f
    };
    bgfx::setUniform(u_texelSize, texelSize);
    bgfx::setUniform(u_outlineColor, m_outlineColor);

    float thicknessData[4] = { m_thickness, 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(u_outlineThickness, thicknessData);

    constexpr uint64_t maskSamplerFlags =
    BGFX_SAMPLER_U_CLAMP |
    BGFX_SAMPLER_V_CLAMP;

    // Bind scene color to slot 0
    bgfx::setTexture(0, u_sceneColor, srcColor);
    // Bind mask only if we have it
    if (hasMask) {
        bgfx::setTexture(1, u_maskTex, ctx.selectionMask, maskSamplerFlags);
    } else {
        // If no mask exists, bind the Scene Color (or a dummy black texture) to Slot 1
        // just so D3D12 doesn't crash/abort due to unbound descriptor.
        bgfx::setTexture(1, u_maskTex, srcColor);
    }

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);

    m_quad.draw(viewId);              // sets VB + IB
    bgfx::submit(viewId, m_program);  // actually draw
}
