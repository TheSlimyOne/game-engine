// CelMaterial.cpp

#include "CelMaterial.h"

#include <iostream>
#include <ostream>
#include <core/MeshData.h>

CelMaterial::CelMaterial(bgfx::ShaderHandle vs, bgfx::ShaderHandle fs, bgfx::ProgramHandle prog) : Material(vs, fs, prog)
{
    // Enable Z-Write and Depth Test (Standard Opaque)
    state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW;

    u_lightDir    = bgfx::createUniform("u_lightDir",    bgfx::UniformType::Vec4);
    u_camPos      = bgfx::createUniform("u_camPos",      bgfx::UniformType::Vec4);
    u_baseColor   = bgfx::createUniform("u_baseColor",   bgfx::UniformType::Vec4);
    u_shadowColor = bgfx::createUniform("u_shadowColor", bgfx::UniformType::Vec4);
    u_toonParams  = bgfx::createUniform("u_toonParams",  bgfx::UniformType::Vec4);
    s_texColor    = bgfx::createUniform("s_texColor",    bgfx::UniformType::Sampler);
    u_texConfig   = bgfx::createUniform("u_texConfig",   bgfx::UniformType::Vec4);
}

CelMaterial::~CelMaterial() {
    bgfx::destroy(u_lightDir);
    bgfx::destroy(u_camPos);
    bgfx::destroy(u_baseColor);
    bgfx::destroy(u_shadowColor);
    bgfx::destroy(u_toonParams);
    bgfx::destroy(s_texColor);
}

void CelMaterial::set_texture(bgfx::TextureHandle handle) { m_textureHandle = handle; }
void CelMaterial::set_base_color(Vec3 color) { m_baseColor = color; }
void CelMaterial::set_shadow_color(Vec3 color) { m_shadowColor = color; }
void CelMaterial::set_light_direction(Vec3 direction) { m_lightDir = direction.normalized(); }
void CelMaterial::set_camera_position(Vec3 pos) { m_camPos = pos; }
void CelMaterial::set_toon_params(float threshold, float softness, float rimStrength, float rimWidth) {
    m_params = Vec4(threshold, softness, rimStrength, rimWidth);
}

void CelMaterial::submit(bgfx::ViewId viewId, const MeshData &mesh, const Mat4 &worldTransform) {

    // Prepare data
    float lightDirData[4] = { m_lightDir.x, m_lightDir.y, m_lightDir.z, 0.0f };
    float camPosData[4]   = { m_camPos.x, m_camPos.y, m_camPos.z, 0.0f };
    float baseColorData[4] = { m_baseColor.x, m_baseColor.y, m_baseColor.z, 1.0f };
    float shadowColorData[4] = { m_shadowColor.x, m_shadowColor.y, m_shadowColor.z, 1.0f };
    float paramsData[4] = { m_params.x, m_params.y, m_params.z, m_params.w };

    bgfx::setUniform(u_lightDir, lightDirData);
    bgfx::setUniform(u_camPos, camPosData);
    bgfx::setUniform(u_baseColor, baseColorData);
    bgfx::setUniform(u_shadowColor, shadowColorData);
    bgfx::setUniform(u_toonParams, paramsData);
    float texConfigData[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (bgfx::isValid(m_textureHandle)) {
        texConfigData[0] = 1.0f; // Set Flag to TRUE
        bgfx::setTexture(0, s_texColor, m_textureHandle);
    }

    bgfx::setUniform(u_texConfig, texConfigData); // this is the "has texture" flag

    bgfx::setTransform(worldTransform.ptr());
    bgfx::setVertexBuffer(0, mesh.vbh);
    bgfx::setIndexBuffer(mesh.ibh);
    bgfx::setState(state);
    bgfx::submit(viewId, program);
}