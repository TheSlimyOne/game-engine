// TextureManager.cpp

#include "TextureManager.h"
#include "Texture.h"

#include <iostream>
#include <bgfx/bgfx.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

std::shared_ptr<Texture> TextureManager::load(const std::string& file_path) {
    // 1. Check Cache
    auto it = m_texture_cache.find(file_path);
    if (it != m_texture_cache.end()) {
        if (auto shared = it->second.lock()) {
            return shared;
        } else {
            m_texture_cache.erase(it); // Clean up expired pointer
        }
    }

    // 2. Load from disk
    // std::cout << "Loading Texture: " << file_path << std::endl;
    auto new_texture = load_from_file(file_path);

    if (new_texture) {
        m_texture_cache[file_path] = new_texture;
    }

    return new_texture;
}

std::shared_ptr<Texture> TextureManager::load_from_file(const std::string& file_path) {
    int width, height, channels;
    
    // stbi_load(filename, x, y, channels_in_file, desired_channels)
    // We force 4 channels (RGBA) to make it easy for BGFX
    // stbi_set_flip_vertically_on_load(true); // OpenGL/BGFX usually expects Y-up
    unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &channels, 4);

    if (!data) {
        std::cerr << "TextureManager: Failed to load image " << file_path 
                  << " Reason: " << stbi_failure_reason() << std::endl;
        return nullptr;
    }

    // Allocate a Texture struct
    auto texture = std::make_shared<Texture>();
    texture->width = static_cast<uint16_t>(width);
    texture->height = static_cast<uint16_t>(height);

    // Copy data to BGFX memory
    // width * height * 4 bytes (RGBA)
    const bgfx::Memory* mem = bgfx::copy(data, width * height * 4);
    
    // We can free the STB data immediately after copying it to BGFX
    stbi_image_free(data);

    // Create the Texture in BGFX
    texture->handle = bgfx::createTexture2D(
        texture->width,
        texture->height,
        false, // hasMips (set to true if you want mipmaps, but you need to generate them)
        1,     // numLayers
        bgfx::TextureFormat::RGBA8, // We forced 4 channels
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
        mem
    );

    if (!bgfx::isValid(texture->handle)) {
        std::cerr << "TextureManager: Failed to create BGFX texture handle for " << file_path << std::endl;
        return nullptr;
    }

    return texture;
}