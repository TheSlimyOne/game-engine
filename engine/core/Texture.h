// Texture.h

#ifndef GAME_TEXTURE_H
#define GAME_TEXTURE_H

#include <bgfx/bgfx.h>

struct Texture {
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
    uint16_t width = 0;
    uint16_t height = 0;

    // Destructor automatically cleans up the BGFX resource
    // when the last shared_ptr to this object is destroyed.
    ~Texture() {
        if (bgfx::isValid(handle)) {
            bgfx::destroy(handle);
        }
    }
};

#endif //GAME_TEXTURE_H