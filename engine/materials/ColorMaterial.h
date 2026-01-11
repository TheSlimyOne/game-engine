#ifndef GAME_COLORMATERIAL_H
#define GAME_COLORMATERIAL_H

#include "Material.h"
#include <bgfx/bgfx.h>
#include <core/Types.h>

class ColorMaterial : public Material {
public:
    ColorMaterial(bgfx::ShaderHandle vs, bgfx::ShaderHandle fs, bgfx::ProgramHandle prog);
    ~ColorMaterial();

    void set_color(float r, float g, float b, float a = 1.0f);

    void submit(bgfx::ViewId viewId, const MeshData& mesh, const Mat4& worldTransform) override;
    void submit_instanced(bgfx::ViewId viewId, const MeshData& mesh, bgfx::InstanceDataBuffer* idb, uint32_t instanceCount) override;

private:
    bgfx::UniformHandle u_color{ BGFX_INVALID_HANDLE };
};


#endif //GAME_COLORMATERIAL_H