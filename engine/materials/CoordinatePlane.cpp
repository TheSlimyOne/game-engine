// CoordinatePlane.cpp

#include "CoordinatePlaneMaterial.h"

#include <core/MeshData.h>

CoordinatePlaneMaterial::CoordinatePlaneMaterial(bgfx::ShaderHandle vs, bgfx::ShaderHandle fs, bgfx::ProgramHandle prog) : Material(vs, fs, prog) {
    set_instanced(false);
    set_blending(true);
    set_backface_culling(false);

    u_gridParams    = bgfx::createUniform("u_gridParams",    bgfx::UniformType::Vec4);
    u_gridPlane     = bgfx::createUniform("u_gridPlane",     bgfx::UniformType::Vec4);
    u_gridColor     = bgfx::createUniform("u_gridColor",     bgfx::UniformType::Vec4);
    u_xAxisPosColor = bgfx::createUniform("u_xAxisPosColor", bgfx::UniformType::Vec4);
    u_xAxisNegColor = bgfx::createUniform("u_xAxisNegColor", bgfx::UniformType::Vec4);
    u_zAxisPosColor = bgfx::createUniform("u_zAxisPosColor", bgfx::UniformType::Vec4);
    u_zAxisNegColor = bgfx::createUniform("u_zAxisNegColor", bgfx::UniformType::Vec4);
}

CoordinatePlaneMaterial::~CoordinatePlaneMaterial() {
    // Clean up handles
    bgfx::destroy(u_gridParams);
    bgfx::destroy(u_gridPlane);
    bgfx::destroy(u_gridColor);
    bgfx::destroy(u_xAxisPosColor);
    bgfx::destroy(u_xAxisNegColor);
    bgfx::destroy(u_zAxisPosColor);
    bgfx::destroy(u_zAxisNegColor);
}


void CoordinatePlaneMaterial::submit(bgfx::ViewId viewId, const MeshData &mesh, const Mat4 &worldTransform) {
    // 2. Define the data values

    // x = Grid Scale (Cell Size), y = Line Thickness, z = Fade Distance, w = Axis Width
    // Note: Fade Distance of 50.0 ensures it fades out before clipping plane
    float gridParams[4] = { 1.0f, 1.0f, 100.0f, 0.1f };

    // x = Plane Scale (This was likely 0 before, causing the mesh to disappear)
    // Make sure your Plane.fbx is roughly 1 unit wide, or adjust this.
    float gridPlane[4]  = { 100.0f, 0.0f, 0.0f, 0.0f };

    // RGBA colors
    float gridColor[4]     = { 0.5f, 0.5f, 0.5f, 0.5f }; // Grey
    float xAxisPos[4]      = { 1.0f, 0.0f, 0.0f, 1.0f }; // Red
    float xAxisNeg[4]      = { 1.0f, 0.0f, 0.0f, 0.5f }; // Red (faded)
    float zAxisPos[4]      = { 0.0f, 0.0f, 1.0f, 1.0f }; // Blue
    float zAxisNeg[4]      = { 0.0f, 0.0f, 1.0f, 0.5f }; // Blue (faded)

    // 3. Set the uniforms
    bgfx::setUniform(u_gridParams, gridParams);
    bgfx::setUniform(u_gridPlane, gridPlane);
    bgfx::setUniform(u_gridColor, gridColor);
    bgfx::setUniform(u_xAxisPosColor, xAxisPos);
    bgfx::setUniform(u_xAxisNegColor, xAxisNeg);
    bgfx::setUniform(u_zAxisPosColor, zAxisPos);
    bgfx::setUniform(u_zAxisNegColor, zAxisNeg);

    // 4. Call base functionality (Transform, buffers, state, submit)
    // Note: We manually do what Material::submit does because we need to inject uniforms
    // between setTransform and submit.
    bgfx::setTransform(worldTransform.ptr());
    bgfx::setVertexBuffer(0, mesh.vbh);
    bgfx::setIndexBuffer(mesh.ibh);
    bgfx::setState(state);

    bgfx::submit(viewId, program);
}