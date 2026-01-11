// Material.h

#ifndef GAME_MATERIAL_H
#define GAME_MATERIAL_H

#include <core/Types.h>
#include <bgfx/bgfx.h>


class MeshData;

const static uint64_t DEFAULT_STATE =
    BGFX_STATE_WRITE_RGB |
    BGFX_STATE_WRITE_A   |
    BGFX_STATE_WRITE_Z   |
    BGFX_STATE_DEPTH_TEST_LESS |
    BGFX_STATE_CULL_CW;

class Material {
public:
    Material() = default;
    Material(bgfx::ShaderHandle vs, bgfx::ShaderHandle fs, bgfx::ProgramHandle prog, uint64_t initialState = DEFAULT_STATE);
    virtual ~Material();

    virtual bool supports_instancing() const;
    virtual void submit(bgfx::ViewId viewId, const MeshData& mesh, const Mat4& worldTransform);
    virtual void submit_instanced(bgfx::ViewId viewId,const MeshData& mesh, bgfx::InstanceDataBuffer* idb,uint32_t instanceCount);

    bgfx::ProgramHandle get_program() const;
    void set_backface_culling(bool enabled);
    void set_blending(bool enabled);

protected:
    bool m_instanced = false;

    bgfx::ShaderHandle vsh{BGFX_INVALID_HANDLE}; // Vertex shader handle
    bgfx::ShaderHandle fsh{BGFX_INVALID_HANDLE}; // Fragment shader handle
    bgfx::ProgramHandle program{BGFX_INVALID_HANDLE}; // Linked program

    // Common fixed pipeline state for this material
    uint64_t state = DEFAULT_STATE;

    void set_instanced(bool enabled);
};

#endif //GAME_MATERIAL_H