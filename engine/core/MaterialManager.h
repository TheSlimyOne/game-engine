// MaterialManager.h

#ifndef GAME_MATERIALMANAGER_H
#define GAME_MATERIALMANAGER_H

#include <bgfx/bgfx.h>
#include <memory>
#include <string>
#include <unordered_map>

class Material;

class MaterialManager {
public:
    std::shared_ptr<Material> load_from_id(const std::string& material_id);

private:
    std::unordered_map<std::string, std::weak_ptr<Material>> m_material_cache;
};


#endif //GAME_MATERIALMANAGER_H