// MeshData.h

#ifndef GAME_MESHCOMPONENT_H
#define GAME_MESHCOMPONENT_H

#include <interfaces/IComponent.h>
#include <bgfx/bgfx.h>

class MeshData {
public:
    bgfx::VertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;

    // Default constructor initializes handles as invalid.
    MeshData() : vbh(BGFX_INVALID_HANDLE), ibh(BGFX_INVALID_HANDLE) {}

    // Convenience constructor to create a component with valid handles.
    MeshData(bgfx::VertexBufferHandle _vbh, bgfx::IndexBufferHandle _ibh)
        : vbh(_vbh), ibh(_ibh) {}

    ~MeshData() {
        if (bgfx::isValid(vbh)) {
            bgfx::destroy(vbh);
        }

        if (bgfx::isValid(ibh)) {
            bgfx::destroy(ibh);
        }
    }
};


#endif //GAME_MESHCOMPONENT_H