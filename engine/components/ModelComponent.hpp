// ModelComponent.hpp

#ifndef GAME_MODELCOMPONENT_H
#define GAME_MODELCOMPONENT_H

#include <memory>
#include <interfaces/IComponent.h>

class MeshData;
class Material;
class Texture;

class ModelComponent : public IComponent {
public:
    std::shared_ptr<MeshData> mesh;
    std::shared_ptr<Material> material;
    std::shared_ptr<Texture> texture;
};



#endif //GAME_MODELCOMPONENT_H