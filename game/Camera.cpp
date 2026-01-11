#include <Engine.h>
#include <iostream>
#include <core/Paths.h>
#include <vector>
#include <random>


void camera_startup(World& world) {
    // 1. Create a main viewport camera
    Entity cam = world.create_entity();
    world.add_component(cam, ComponentType::Transform);
    world.add_component(cam, ComponentType::Camera);
    world.set_active_camera(cam);
    world.set_position(cam, Vec3(0.0f, 5.0f, 50.0f));

    world.add_input_action("SelectEntity", MouseButton::Left);
    world.add_input_action("PanCamera", MouseButton::Right);
    world.add_input_action("CameraMoveForward", KeyCode::W);
    world.add_input_action("CameraMoveBackward", KeyCode::S);
    world.add_input_action("CameraMoveLeft", KeyCode::A);
    world.add_input_action("CameraMoveRight", KeyCode::D);
    world.add_input_action("CameraMoveUp", KeyCode::E);
    world.add_input_action("CameraMoveDown", KeyCode::Q);
    world.add_input_action("SpeedUpCamera", KeyCode::LeftShift);

    Entity cp = world.create_entity();
    world.add_component(cp, ComponentType::Transform);
    world.add_component(cp, ComponentType::Model);
    world.set_rotation(cp, Vec3(radians(-90.0f), 0.0f, 0.0f));
    world.load_mesh(cp, (ROOT / "models/Plane.fbx").string());
    world.load_material(cp, "coordinate_plane");
    world.set_backface_culling(cp, false);
    world.set_alpha_blending(cp, true);
}

struct CameraState {
    float yaw = 0.0f;
    float pitch = 0.0f;
};

void camera_update(World& world, float dt, CameraState& state) {
    // 1) Apply pick result from *previous* frame (if any).
    if (world.has_pick_result()) {
        Entity picked = world.consume_pick_result();
        std::cout << "PICKED: " << picked << std::endl;
        bool additive = world.input_action_held("SelectAdd");

        if (picked) {
            world.select_entity(picked, additive);
        } else if (!additive) {
            // Clicked empty space: clear selection unless additive.
            world.clear_selection();
        }
    }

    Entity cam = world.get_active_camera();

    // 2) Handle selection request this frame.
    if (world.input_action_pressd("SelectEntity")) {     // note your API spelling
        int mouseX = 0, mouseY = 0;
        world.get_mouse_screen_pos(mouseX, mouseY);
        std::cout << mouseX << ", " << mouseY << std::endl;

        world.request_pick(mouseX, mouseY);
    }

    // Camera Logic
    if (world.input_action_held("PanCamera")) {
        world.set_cursor_mode(CursorMode::Disabled);
        Vec2 delta = world.get_mouse_delta();
        state.yaw -= delta.x * 0.01f;
        state.pitch -= delta.y * 0.01f;
        Quat q_yaw = Quat::angle_axis(state.yaw, Vec3(0.0f, 1.0f, 0.0f));
        Quat q_pitch = Quat::angle_axis(state.pitch, Vec3(1.0f, 0.0f, 0.0f));
        world.set_rotation(cam, q_yaw * q_pitch);
    }

    if (world.input_action_released("PanCamera")) {
        world.set_cursor_mode(CursorMode::Normal);
    }

    Vec3 translate = Vec3(0.0f);
    Vec3 forward = world.get_forward(cam);
    Vec3 right = world.get_right(cam);
    Vec3 up = world.get_up(cam);
    float cam_speed = 7.0f;

    if (world.input_action_held("CameraMoveForward")) translate += forward;
    if (world.input_action_held("CameraMoveBackward")) translate -= forward;
    if (world.input_action_held("CameraMoveLeft")) translate -= right;
    if (world.input_action_held("CameraMoveRight")) translate += right;
    if (world.input_action_held("CameraMoveUp")) translate += up;
    if (world.input_action_held("CameraMoveDown")) translate -= up;
    if (world.input_action_held("SpeedUpCamera")) cam_speed = 50.0f;

    Vec3 cur_pos = world.get_position(cam);
    world.set_position(cam, cur_pos + translate * dt * cam_speed);
}

SYSTEMS_ON_STARTUP(camera_startup);
SYSTEMS_ON_UPDATE(camera_update, CameraState);