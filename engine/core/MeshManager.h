// MeshManager.h

#ifndef GAME_MESHMANAGER_H
#define GAME_MESHMANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

class MeshData;

class MeshManager {
public:
    std::shared_ptr<MeshData> load(const std::string& file_path);

private:
    std::shared_ptr<MeshData> load_from_file(const std::string& file_path);

    std::unordered_map<std::string, std::weak_ptr<MeshData>> m_mesh_cache;
};


#endif //GAME_MESHMANAGER_H