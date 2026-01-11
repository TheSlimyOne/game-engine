// TextureManager.h

#ifndef GAME_TEXTUREMANAGER_H
#define GAME_TEXTUREMANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

struct Texture; // Forward declaration

class TextureManager {
public:
    // Retrieves a texture from cache or loads it from disk if not found.
    std::shared_ptr<Texture> load(const std::string& file_path);

private:
    std::shared_ptr<Texture> load_from_file(const std::string& file_path);

    // Cache using weak_ptr to allow automatic unloading of unused assets
    std::unordered_map<std::string, std::weak_ptr<Texture>> m_texture_cache;
};

#endif //GAME_TEXTUREMANAGER_H