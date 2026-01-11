// CelMaterial.h

#ifndef GAME_CELMATERIAL_H
#define GAME_CELMATERIAL_H

#include "Material.h"
#include <bgfx/bgfx.h>
#include <core/Types.h>

class CelMaterial : public Material {
public:
    CelMaterial(bgfx::ShaderHandle vs, bgfx::ShaderHandle fs, bgfx::ProgramHandle prog);
    ~CelMaterial() override;

    void submit(bgfx::ViewId viewId, const MeshData& mesh, const Mat4& worldTransform) override;

    void set_texture(bgfx::TextureHandle handle);
    void set_base_color(Vec3 color);
    void set_shadow_color(Vec3 color);
    void set_light_direction(Vec3 direction);
    void set_camera_position(Vec3 pos);
    void set_toon_params(float threshold, float softness, float rimStrength, float rimWidth);

private:
    bgfx::UniformHandle u_lightDir;
    bgfx::UniformHandle u_camPos;
    bgfx::UniformHandle u_baseColor;
    bgfx::UniformHandle u_shadowColor;
    bgfx::UniformHandle u_toonParams;
    bgfx::UniformHandle s_texColor;
    bgfx::UniformHandle u_texConfig;

    bgfx::TextureHandle m_textureHandle = BGFX_INVALID_HANDLE;

    // Local storage of values
    Vec3 m_lightDir = Vec3(0.5f, 1.0f, 0.5f); // Default diagonal light
    Vec3 m_camPos = Vec3(0.0f, 0.0f, 0.0f);
    Vec3 m_baseColor = Vec3(0.8f); // White
    Vec3 m_shadowColor = Vec3(0.7f, 0.7f, 0.8f); // Nice cool blue shadow
    Vec4 m_params = Vec4(0.5f, 0.02f, 0.5f, 0.6f);
};

#endif //GAME_CELMATERIAL_H