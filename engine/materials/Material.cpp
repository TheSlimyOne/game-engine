// Material.cpp

#include "Material.h"

#include <iostream>
#include <ostream>

#include "../core/MeshData.h"

Material::Material(bgfx::ShaderHandle vs, bgfx::ShaderHandle fs, bgfx::ProgramHandle prog, uint64_t initialState) :
    vsh(vs),
    fsh(fs),
    program(prog),
    state(initialState)
{}

Material::~Material() {
    if (bgfx::isValid(program)) {
        bgfx::destroy(program);
    }
    if (bgfx::isValid(vsh)) {
        bgfx::destroy(vsh);
    }
    if (bgfx::isValid(fsh)) {
        bgfx::destroy(fsh);
    }
}

bool Material::supports_instancing() const {
    return m_instanced;
}

void Material::submit(bgfx::ViewId viewId, const MeshData &mesh, const Mat4 &worldTransform) {
    bgfx::setTransform(worldTransform.ptr());
    bgfx::setVertexBuffer(0, mesh.vbh);
    bgfx::setIndexBuffer(mesh.ibh);
    bgfx::setState(state);
    bgfx::submit(viewId, program);
}


void Material::submit_instanced(bgfx::ViewId viewId, const MeshData &mesh, bgfx::InstanceDataBuffer *idb, uint32_t instanceCount) {
    bgfx::setVertexBuffer(0, mesh.vbh);
    bgfx::setIndexBuffer(mesh.ibh);

    if (idb && instanceCount > 0) {
        bgfx::setInstanceDataBuffer(idb, 0, instanceCount);
    }

    bgfx::setState(state);
    bgfx::submit(viewId, program);
}

bgfx::ProgramHandle Material::get_program() const {
    return program;
}


void Material::set_backface_culling(bool enabled) {
    if (enabled) {
        state |= BGFX_STATE_CULL_CW;
    } else {
        state &= ~BGFX_STATE_CULL_CW;
    }
}

void Material::set_blending(bool enabled) {
    if (enabled) {
        // Standard Alpha Blending: (SrcAlpha, InvSrcAlpha)
        state |= BGFX_STATE_BLEND_ALPHA;
    } else {
        state &= ~BGFX_STATE_BLEND_ALPHA;
    }
}

void Material::set_instanced(bool enabled) {
    m_instanced = enabled;
}






