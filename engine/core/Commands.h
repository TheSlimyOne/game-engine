#ifndef GAME_COMMANDS_H
#define GAME_COMMANDS_H

#include "Types.h"

struct CmdInsertEntity {
    Entity entity;
};

struct CmdRegisterTransform {
    Entity entity;
};

struct CmdAddCameraComponent {
    Entity entity;
};

struct CmdAddModelComponent {
    Entity entity;
};

struct CmdSetActiveCamera {
    Entity entity;
};

struct CmdSetPosition {
    Entity entity;
    Vec3   pos;
};

struct CmdSetRotationQuat {
    Entity entity;
    Quat   rot;
};

struct CmdSetScale {
    Entity entity;
    Vec3   scale;
};

struct CmdSetParent {
    Entity entity;
    Entity parent;
};

struct CmdLoadModelMesh {
    Entity entity;
    std::string file_path;
};

struct CmdLoadModelMaterial {
    Entity        entity;
    std::string material_id;
};

struct CmdLoadModelTexture {
    Entity entity;
    std::string file_path;
};

struct CmdSetBackfaceCulling {
    Entity entity;
    bool   enabled;
};

struct CmdSetAlphaBlending {
    Entity entity;
    bool   enabled;
};

#endif //GAME_COMMANDS_H