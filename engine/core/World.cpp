// World.cpp

#include "World.h"
#include <iostream>

#include "Texture.h"
#include "materials/CelMaterial.h"

World::World()
    :   m_active_camera(0),
        m_command_queue(std::make_unique<CommandQueue>()),
        m_entity_pool(std::make_unique<EntityPool>(MAX_ENTITIES)),
        m_entity_sparse_set(std::make_unique<EntitySparseSet>(MAX_ENTITIES)),
        m_input_manager(std::make_unique<InputManager>()),
        m_mesh_manager(std::make_unique<MeshManager>()),
        m_material_manager(std::make_unique<MaterialManager>()),
        m_texture_manager(std::make_unique<TextureManager>()),
        m_transform_manager(std::make_unique<TransformManager>(MAX_ENTITIES)),

        m_camera_component_pool(std::make_unique<ComponentPool<CameraComponent>>()),
        m_model_component_pool(std::make_unique<ComponentPool<ModelComponent>>())
{}

// =================== General World Interface =================== //
Entity World::create_entity() {
    Entity entity = m_entity_pool->create();
    m_command_queue->submit(CmdInsertEntity{entity});
    return entity;
}

void World::execute_commands() {
    m_command_queue->for_each_and_clear([this](Command& cmd) {
        std::visit([this](auto& c) {
            execute_command(c);
        }, cmd);
    });
}

void World::add_component(Entity entity, ComponentType component_type) {
    switch (component_type) {
        case ComponentType::Transform:
            m_command_queue->submit(CmdRegisterTransform{entity});
            break;
        case ComponentType::Camera:
            m_command_queue->submit(CmdAddCameraComponent{entity});
            break;
        case ComponentType::Model:
            m_command_queue->submit(CmdAddModelComponent{entity});
            break;
    }
}

void World::update_transforms() {
    m_transform_manager->update_transforms();
}

// =============================================================== //


// ======================= Camera Interface ====================== //
Entity World::get_active_camera() const {
    return m_active_camera;
}

Mat4 World::get_camera_view_matrix(Entity camera) {
    Mat4 cam_world_transform = m_transform_manager->get_world_transform(camera);
    return Mat4::view_from_transform(cam_world_transform);
}

Mat4 World::get_camera_proj_matrix(Entity camera) {
    CameraComponent* camera_comp = m_camera_component_pool->get(camera);

    return !camera_comp ? Mat4::identity() : camera_comp->get_projection_matrix();
}

bgfx::ViewId World::get_gamera_proj_view_id(Entity camera) {
    CameraComponent* camera_comp = m_camera_component_pool->get(camera);

    return !camera_comp ? 0 : camera_comp->view_id;
}

uint16_t World::get_camera_clear_flags(Entity camera) {
    CameraComponent* camera_comp = m_camera_component_pool->get(camera);

    return !camera_comp ? 0 : camera_comp->clear_flags;
}

void World::set_active_camera(Entity entity) {
    m_command_queue->submit(CmdSetActiveCamera{entity});
}
// =============================================================== //


// ======================= Model Interface ======================= //
void World::load_mesh(Entity entity, const std::string &file_path) {
    m_command_queue->submit(CmdLoadModelMesh{entity, file_path});
}

void World::load_material(Entity entity, const std::string &material_id) {
    m_command_queue->submit(CmdLoadModelMaterial{entity, material_id});
}

void World::set_backface_culling(Entity entity, bool enabled) {
    m_command_queue->submit(CmdSetBackfaceCulling{entity, enabled});
}

void World::set_alpha_blending(Entity entity, bool enabled) {
    m_command_queue->submit(CmdSetAlphaBlending{entity, enabled});
}

void World::load_texture(Entity entity, const std::string &file_path) {
    m_command_queue->submit(CmdLoadModelTexture{entity, file_path});
}


// =============================================================== //


// =================== Input Manager Interface =================== //
void World::query_inputs() {
    m_input_manager->query_inputs();
}

void World::add_input_action(const std::string &name, KeyCode key) {
    m_input_manager->add_action(name, key);
}

void World::add_input_action(const std::string &name, MouseButton btn) {
    m_input_manager->add_action(name, btn);
}

bool World::input_action_pressd(const std::string &name) {
    return m_input_manager->is_action_pressed(name);
}

bool World::input_action_held(const std::string &name) {
    return m_input_manager->is_action_held(name);
}

bool World::input_action_released(const std::string &name) {
    return m_input_manager->is_action_released(name);
}

Vec2 World::get_mouse_delta() {
    return m_input_manager->get_mouse_delta();
}

void World::get_mouse_screen_pos(int &x, int &y) {
    m_input_manager->get_mouse_position_int(x, y);
}

void World::set_cursor_mode(CursorMode mode) {
    m_input_manager->set_cursor_mode(mode);
}

void World::set_input_manager_window(const std::shared_ptr<Window> &window) {
    m_input_manager->set_window(window);
}
// =============================================================== //


// ====================== Transform Manager Interface ==================== //
// GETTERS
Vec3 World::get_forward(Entity entity) {
    Mat4 transform = m_transform_manager->get_world_transform(entity);
    return transform.forward();
}

Vec3 World::get_right(Entity entity) {
    Mat4 transform = m_transform_manager->get_world_transform(entity);
    return transform.right();
}

Vec3 World::get_up(Entity entity) {
    Mat4 transform = m_transform_manager->get_world_transform(entity);
    return transform.up();
}

Vec3 World::get_position(Entity entity) {
    return m_transform_manager->get_position(entity);
}

Mat4 World::get_world_transform(Entity entity) {
    return m_transform_manager->get_world_transform(entity);
}

// SETTERS
void World::set_position(Entity entity, Vec3 pos) {
    m_command_queue->submit(CmdSetPosition{entity, pos});
}

void World::set_rotation(Entity entity, Vec3 angle_rot) {
    Quat rot = Quat::from_euler(angle_rot);
    m_command_queue->submit(CmdSetRotationQuat{entity, rot});
}

void World::set_rotation(Entity entity, Quat rot) {
    m_command_queue->submit(CmdSetRotationQuat{entity, rot});
}

void World::set_rotation(Entity entity, Vec3 axis, float angle_rad) {
    Quat current = m_transform_manager->get_rotation(entity);
    Quat delta = Quat::angle_axis(angle_rad, axis);
    m_command_queue->submit(CmdSetRotationQuat{entity, current * delta});
}

void World::set_scale(Entity entity, Vec3 scale) {
    m_command_queue->submit(CmdSetScale{entity, scale});
}

void World::set_parent(Entity entity, Entity parent) {
    m_command_queue->submit(CmdSetParent{entity, parent});
}

// =============================================================== //

// ===================== Selection Interface ===================== //
void World::select_entity(Entity entity, bool additive) {
    // Treat 0 as "no entity" (you already do this for cameras).
    if (entity == 0) {
        if (!additive) {
            m_selected_entities.clear();
        }
        return;
    }

    // If not doing multi-select, clear previous selection.
    if (!additive) {
        m_selected_entities.clear();
    }

    m_selected_entities.insert(entity);
}

void World::deselect_entity(Entity entity) {
    if (entity == 0) {
        return;
    }

    m_selected_entities.erase(entity);
}

void World::clear_selection() {
    m_selected_entities.clear();
}

bool World::is_entity_selected(Entity entity) const {
    if (entity == 0) {
        return false;
    }

    auto it = m_selected_entities.find(entity);
    return it != m_selected_entities.end();
}

void World::request_pick(int mouseX, int mouseY) {
    m_pickRequest.requested = true;
    m_pickRequest.mouseX    = mouseX;
    m_pickRequest.mouseY    = mouseY;
}

bool World::has_pick_result() const {
    return m_pickResult.hasResult;
}

Entity World::consume_pick_result() {
    if (!m_pickResult.hasResult) {
        return 0;
    }
    m_pickResult.hasResult = false;
    Entity e = m_pickResult.entity;
    m_pickResult.entity = 0;
    return e;
}
// =============================================================== //


// ================= Command Execution Interface ================= //
void World::execute_command(CmdInsertEntity& cmd) {
    m_entity_sparse_set->insert(cmd.entity);
}

void World::execute_command(CmdRegisterTransform& cmd) {
    m_transform_manager->register_entity(cmd.entity);
}

void World::execute_command(CmdAddCameraComponent &cmd) {
    m_camera_component_pool->add(cmd.entity, std::make_unique<CameraComponent>());
}

void World::execute_command(CmdAddModelComponent &cmd) {
    m_model_component_pool->add(cmd.entity, std::make_unique<ModelComponent>());
}

void World::execute_command(CmdSetActiveCamera &cmd) {
    m_active_camera = cmd.entity;
}

void World::execute_command(CmdSetPosition &cmd) {
    m_transform_manager->set_position(cmd.entity, cmd.pos);
}

void World::execute_command(CmdSetRotationQuat &cmd) {
    m_transform_manager->set_rotation(cmd.entity, cmd.rot);
}

void World::execute_command(CmdSetScale &cmd) {
    m_transform_manager->set_scale(cmd.entity, cmd.scale);
}

void World::execute_command(CmdSetParent &cmd) {
    m_transform_manager->set_parent(cmd.entity, cmd.parent);
}

void World::execute_command(CmdLoadModelMesh &cmd) {
    ModelComponent* model_comp = m_model_component_pool->get(cmd.entity);

    if (!model_comp) {
        std::cerr << "Entity does not contain a model component." << std::endl;
        return;
    }

    model_comp->mesh = m_mesh_manager->load(cmd.file_path);
}

void World::execute_command(CmdLoadModelMaterial &cmd) {
    ModelComponent* model_comp = m_model_component_pool->get(cmd.entity);

    if (!model_comp) {
        std::cerr << "Entity does not contain a model component." << std::endl;
        return;
    }

    model_comp->material = m_material_manager->load_from_id(cmd.material_id);
}

void World::execute_command(CmdLoadModelTexture &cmd) {
    ModelComponent* model_comp = m_model_component_pool->get(cmd.entity);

    if (!model_comp) {
        std::cerr << "[World] Error: Entity does not contain a model component." << std::endl;
        return;
    }

    if (!model_comp->material) {
        std::cerr << "[World] Error: Cannot load texture. Entity has no material assigned yet." << std::endl;
        return;
    }

    // 1. Load the texture with safety check
    std::shared_ptr<Texture> texture = m_texture_manager->load(cmd.file_path);
    if (!texture) {
        std::cerr << "[World] Error: Failed to load texture: " << cmd.file_path << std::endl;
        return; // STOP here to prevent crash
    }

    model_comp->texture = texture;

    // 2. Safe Dynamic Cast
    // We check if the material is actually capable of accepting a texture (is it a CelMaterial?)
    auto cel_material = dynamic_cast<CelMaterial*>(model_comp->material.get());

    if (cel_material) {
        // Success: It is a CelMaterial
        cel_material->set_texture(texture->handle);
    } else {
        // Failure: It's a different type of material
        std::cerr << "[World] Warning: Material on entity is not 'CelMaterial'. Texture ignored." << std::endl;
    }
}


void World::execute_command(CmdSetBackfaceCulling &cmd) {
    ModelComponent* model_comp = m_model_component_pool->get(cmd.entity);

    if (!model_comp) {
        std::cerr << "Entity does not contain a model component." << std::endl;
        return;
    }

    model_comp->material->set_backface_culling(cmd.enabled);
}

void World::execute_command(CmdSetAlphaBlending &cmd) {
    ModelComponent* model_comp = m_model_component_pool->get(cmd.entity);

    if (!model_comp || !model_comp->material) {
        std::cerr << "Entity does not have a valid material." << std::endl;
        return;
    }

    model_comp->material->set_blending(cmd.enabled);
}
// =============================================================== //