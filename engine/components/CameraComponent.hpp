#ifndef GAME_CAMERACOMPONENT_H
#define GAME_CAMERACOMPONENT_H

#include <interfaces/IComponent.h>
#include <bgfx/bgfx.h>
#include <core/Types.h>


class CameraComponent : public IComponent {
public:
    bgfx::ViewId view_id = 0;
    uint16_t clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;
    ProjectionType projection_type = ProjectionType::Perspective;
    float aspect_ratio = 16.0f / 9.0f;

    // --- Perspective Properties ---
    float fov_degrees = 60.0f;
    float near_clip = 0.1f;
    float far_clip = 1000.0f;

    // --- Orthographic Properties ---
    float ortho_size = 10.0f;

    Mat4 get_projection_matrix();
};

#endif //GAME_CAMERACOMPONENT_H