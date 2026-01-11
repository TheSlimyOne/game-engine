// PostProcessingChain.cpp

#include "PostProcessChain.h"
#include <bx/bx.h>

PostProcessChain::~PostProcessChain() {
    shutdown();
}

void PostProcessChain::init(uint16_t width, uint16_t height) {
    m_width = width;
    m_height = height;
    create_ping_pong();
}

void PostProcessChain::shutdown() {
    destroy_ping_pong();
    m_effects.clear();
}

void PostProcessChain::resize(uint16_t width, uint16_t height) {
    if (width == m_width && height == m_height) return;
    m_width = width;
    m_height = height;
    destroy_ping_pong();
    create_ping_pong();

    for (auto& e : m_effects) {
        e->resize(width, height);
    }
}

void PostProcessChain::create_ping_pong() {
    if (m_width == 0 || m_height == 0) return;

    auto makeFb = [this](bgfx::TextureHandle& color, bgfx::FrameBufferHandle& fb) {
        color = bgfx::createTexture2D(
            m_width, m_height,
            false, 1,
            bgfx::TextureFormat::BGRA8,
            BGFX_TEXTURE_RT
        );

        bgfx::TextureHandle attachments[] = { color };
        fb = bgfx::createFrameBuffer(
            BX_COUNTOF(attachments),
            attachments,
            true
        );
    };

    makeFb(m_pingColor, m_pingFb);
    makeFb(m_pongColor, m_pongFb);
}

void PostProcessChain::destroy_ping_pong() {
    if (bgfx::isValid(m_pingFb)) bgfx::destroy(m_pingFb);
    if (bgfx::isValid(m_pongFb)) bgfx::destroy(m_pongFb);

    m_pingFb = BGFX_INVALID_HANDLE;
    m_pongFb = BGFX_INVALID_HANDLE;
    m_pingColor = BGFX_INVALID_HANDLE;
    m_pongColor = BGFX_INVALID_HANDLE;
}

void PostProcessChain::add_effect(std::unique_ptr<PostProcessEffect> effect) {
    m_effects.push_back(std::move(effect));
}

void PostProcessChain::execute(PostProcessContext& ctx, bgfx::TextureHandle sceneColor, bgfx::FrameBufferHandle finalFb) {
    if (m_effects.empty()) {
        return;
    }

    bgfx::TextureHandle currentColor = sceneColor;
    bool usePing = true;

    for (size_t i = 0; i < m_effects.size(); ++i) {
        auto& effect = m_effects[i];
        if (!effect->enabled) continue;

        // find if this is the last enabled effect
        bool isLastEnabled = true;
        for (size_t j = i + 1; j < m_effects.size(); ++j) {
            if (m_effects[j]->enabled) {
                isLastEnabled = false;
                break;
            }
        }

        bgfx::FrameBufferHandle dstFb;
        if (isLastEnabled) {
            // final pass: write to finalFb (may be BGFX_INVALID_HANDLE => backbuffer)
            dstFb = finalFb;
        } else {
            // intermediate pass: use ping-pong buffers
            dstFb = usePing ? m_pingFb : m_pongFb;
        }

        effect->apply(ctx, currentColor, dstFb);

        if (!isLastEnabled) {
            currentColor = usePing ? m_pingColor : m_pongColor;
            usePing = !usePing;
        }
    }
}