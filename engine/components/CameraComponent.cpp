#include "CameraComponent.hpp"


Mat4 CameraComponent::get_projection_matrix() {
    if (projection_type == ProjectionType::Perspective) {
        return Mat4::perspective(
            radians(this->fov_degrees),
            this->aspect_ratio,
            this->near_clip,
            this->far_clip);
    } else {
        float half_height = this->ortho_size * 0.5f;
        float half_width = half_height * this->aspect_ratio;
        return Mat4::orthographic(
            -half_width, half_width,
            -half_height, half_height,
            this->near_clip,
            this->far_clip);
    }
}


