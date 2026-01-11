// CoordinatePlaneMaterial.h

#ifndef GAME_COORDINATEPLANEMATERIAL_H
#define GAME_COORDINATEPLANEMATERIAL_H

#include "Material.h"
#include <bgfx/bgfx.h>

class CoordinatePlaneMaterial : public Material {
public:
    CoordinatePlaneMaterial(bgfx::ShaderHandle vs, bgfx::ShaderHandle fs, bgfx::ProgramHandle prog);
    ~CoordinatePlaneMaterial() override;

    void submit(bgfx::ViewId viewId, const MeshData& mesh, const Mat4& worldTransform) override;
private:
    bgfx::UniformHandle u_gridParams;
    bgfx::UniformHandle u_gridPlane;
    bgfx::UniformHandle u_gridColor;
    bgfx::UniformHandle u_xAxisPosColor;
    bgfx::UniformHandle u_xAxisNegColor;
    bgfx::UniformHandle u_zAxisPosColor;
    bgfx::UniformHandle u_zAxisNegColor;

};

#endif //GAME_COORDINATEPLANEMATERIAL_H