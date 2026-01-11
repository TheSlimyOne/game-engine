// PostProcessChain.h

#ifndef GAME_POSTPROCESSCHAIN_H
#define GAME_POSTPROCESSCHAIN_H

#include "PostProcessEffect.h"

class PostProcessChain {
public:
    PostProcessChain() = default;
    ~PostProcessChain();

    void init(uint16_t width, uint16_t height);
    void shutdown();

    void resize(uint16_t width, uint16_t height);

    void add_effect(std::unique_ptr<PostProcessEffect> effect);

    // sceneColor: output of your main 3D render
    // finalFb: usually BGFX_INVALID_HANDLE to mean backbuffer
    void execute(PostProcessContext& ctx, bgfx::TextureHandle sceneColor, bgfx::FrameBufferHandle finalFb);

private:
    std::vector<std::unique_ptr<PostProcessEffect>> m_effects;

    uint16_t m_width  = 0;
    uint16_t m_height = 0;

    bgfx::FrameBufferHandle m_pingFb = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle     m_pingColor = BGFX_INVALID_HANDLE;

    bgfx::FrameBufferHandle m_pongFb = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle     m_pongColor = BGFX_INVALID_HANDLE;

    void create_ping_pong();
    void destroy_ping_pong();
};

#endif //GAME_POSTPROCESSCHAIN_H