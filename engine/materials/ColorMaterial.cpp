#include "ColorMaterial.h"
#include <core/MeshData.h>

ColorMaterial::ColorMaterial(bgfx::ShaderHandle vs, bgfx::ShaderHandle fs, bgfx::ProgramHandle prog) : Material(vs, fs, prog) {
    set_instanced(true); // this material supports instancing
    u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
}

ColorMaterial::~ColorMaterial() {
    if (bgfx::isValid(u_color)) {
        bgfx::destroy(u_color);
    }
}

void ColorMaterial::set_color(float r, float g, float b, float a) {
    float v[4] = { r, g, b, a };
    bgfx::setUniform(u_color, v);
}

void ColorMaterial::submit(bgfx::ViewId viewId, const MeshData &mesh, const Mat4 &worldTransform) {
    // Bind any per-draw uniforms first
    // (e.g. if color is per-object, call set_color() before this)
    bgfx::setTransform(worldTransform.ptr());
    bgfx::setVertexBuffer(0, mesh.vbh);
    bgfx::setIndexBuffer(mesh.ibh);
    bgfx::setState(state);
    bgfx::submit(viewId, program);
}


void ColorMaterial::submit_instanced(bgfx::ViewId viewId, const MeshData &mesh, bgfx::InstanceDataBuffer *idb, uint32_t instanceCount) {
    bgfx::setVertexBuffer(0, mesh.vbh);
    bgfx::setIndexBuffer(mesh.ibh);

    if (idb && instanceCount > 0) {
        bgfx::setInstanceDataBuffer(idb, 0, instanceCount);
    }

    // color already set with set_color(...) before render loop,
    // or you could add a per-batch setter
    bgfx::setState(state);
    bgfx::submit(viewId, program);
}

