// PostProcessEffect.h

#ifndef GAME_POSTPROCESSEFFECT_H
#define GAME_POSTPROCESSEFFECT_H

#include <bgfx/bgfx.h>
#include <vector>
#include <memory>

#include <core/Types.h>

class World;

struct PostProcessContext {
    uint16_t width  = 0;
    uint16_t height = 0;

    bgfx::ViewId nextViewId = 0;   // increment as effects allocate views

    bgfx::TextureHandle sceneDepth = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle selectionMask = BGFX_INVALID_HANDLE; // used by outline effect etc.

    World* world = nullptr;        // optional, if an effect needs gameplay data
};


class PostProcessEffect {
public:
    virtual ~PostProcessEffect() = default;

    bool enabled = true;

    virtual void resize(uint16_t width, uint16_t height) {}
    virtual void apply(PostProcessContext& ctx, bgfx::TextureHandle srcColor, bgfx::FrameBufferHandle dstFb) = 0;
};

#endif //GAME_POSTPROCESSEFFECT_H