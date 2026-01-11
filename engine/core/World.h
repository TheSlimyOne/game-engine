// World.h

#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include <memory>
#include <bgfx/bgfx.h>
#include <string>
#include <unordered_set>

#include "ComponentPool.h"
#include "CommandQueue.h"
#include "EntitySparseSet.h"
#include "Types.h"
#include "View2.h"
#include "EntityPool.h"
#include "InputManager.h"
#include "MeshManager.h"
#include "MaterialManager.h"
#include "TransformManager.h"
#include "Commands.h"
#include "TextureManager.h"

#include <platform/Window.h>
#include <components/ModelComponent.hpp>
#include <components/CameraComponent.hpp>
#include <materials/Material.h>

struct PickRequest {
    bool requested = false;
    int  mouseX    = 0;
    int  mouseY    = 0;
};

struct PickResult {
    bool   hasResult = false;
    Entity entity    = 0;
};

class World {
public:
    World();

    // =================== General World Interface =================== //
    Entity create_entity();
    void execute_commands();
    void add_component(Entity entity, ComponentType component_type);
    void update_transforms();
    // =============================================================== //


    // ======================= Camera Interface ====================== //
    Entity get_active_camera() const;
    Mat4 get_camera_view_matrix(Entity camera);
    Mat4 get_camera_proj_matrix(Entity camera);
    bgfx::ViewId get_gamera_proj_view_id(Entity camera);
    uint16_t get_camera_clear_flags(Entity camera);
    void set_active_camera(Entity entity);
    // =============================================================== //


    // ======================= Model Interface ======================= //
    void load_mesh(Entity entity, const std::string& file_path);
    void load_material(Entity entity, const std::string& material_id);
    void load_texture(Entity entity, const std::string& file_path);
    void set_backface_culling(Entity entity, bool enabled);
    void set_alpha_blending(Entity entity, bool enabled);
    // =============================================================== //


    // =================== Input Manager Interface =================== //
    void query_inputs();
    void add_input_action(const std::string& name, KeyCode key);
    void add_input_action(const std::string& name, MouseButton btn);
    bool input_action_pressd(const std::string& name);
    bool input_action_held(const std::string& name);
    bool input_action_released(const std::string& name);
    Vec2 get_mouse_delta();
    void get_mouse_screen_pos(int& x, int& y);
    void set_cursor_mode(CursorMode mode);
    void set_input_manager_window(const std::shared_ptr<Window>& window);
    // =============================================================== //


    // ====================== Transform Manager Interface ==================== //
    Vec3 get_forward(Entity entity);
    Vec3 get_right(Entity entity);
    Vec3 get_up(Entity entity);
    Vec3 get_position(Entity entity);
    Mat4 get_world_transform(Entity entity);

    void set_position(Entity entity, Vec3 pos);
    void set_rotation(Entity entity, Vec3 angle_rot);
    void set_rotation(Entity entity, Quat rot);
    void set_rotation(Entity entity, Vec3 axis, float angle_rad);
    void set_scale(Entity entity, Vec3 scale);
    void set_parent(Entity entity, Entity parent);
    // =============================================================== //

    // ===================== Selection Interface ===================== //
    // Replace current selection with this entity (unless additive=true).
    void select_entity(Entity entity, bool additive = false);
    // Remove a single entity from the selection set.
    void deselect_entity(Entity entity);
    // Clear all selected entities.
    void clear_selection();
    // Query if an entity is currently selected.
    bool is_entity_selected(Entity entity) const;

    void request_pick(int mouseX, int mouseY);
    bool has_pick_result() const;
    Entity consume_pick_result();
    // =============================================================== //

    template<typename C> ComponentPool<C>* pool();

    template<typename A, typename B>
    View2<A,B> view() {
        return View2<A, B>(*pool<A>(), *pool<B>());
    }

private:
    Entity m_active_camera;
    std::unique_ptr<CommandQueue> m_command_queue;
    std::unique_ptr<EntityPool> m_entity_pool;
    std::unique_ptr<EntitySparseSet> m_entity_sparse_set;
    std::unordered_set<Entity> m_selected_entities;

    // Managers
    std::unique_ptr<InputManager> m_input_manager;
    std::unique_ptr<MeshManager> m_mesh_manager;
    std::unique_ptr<MaterialManager> m_material_manager;
    std::unique_ptr<TextureManager> m_texture_manager;
    std::unique_ptr<TransformManager> m_transform_manager;

    // Pools
    std::unique_ptr<ComponentPool<CameraComponent>> m_camera_component_pool;
    std::unique_ptr<ComponentPool<ModelComponent>> m_model_component_pool;

    PickRequest m_pickRequest;
    PickResult  m_pickResult;

    // ====================== Command Execution Interface ==================== //
    void execute_command(CmdInsertEntity& cmd);
    void execute_command(CmdRegisterTransform& cmd);
    void execute_command(CmdAddCameraComponent& cmd);
    void execute_command(CmdAddModelComponent& cmd);
    void execute_command(CmdSetActiveCamera& cmd);
    void execute_command(CmdSetPosition& cmd);
    void execute_command(CmdSetRotationQuat& cmd);
    void execute_command(CmdSetScale& cmd);
    void execute_command(CmdSetParent& cmd);
    void execute_command(CmdLoadModelMesh& cmd);
    void execute_command(CmdLoadModelMaterial& cmd);
    void execute_command(CmdLoadModelTexture& cmd);
    void execute_command(CmdSetBackfaceCulling& cmd);
    void execute_command(CmdSetAlphaBlending& cmd);

    // =============================================================== //

    friend class Renderer;
};

template<> inline ComponentPool<CameraComponent>* World::pool<CameraComponent>() {
    return m_camera_component_pool.get();
}
template<> inline ComponentPool<ModelComponent>* World::pool<ModelComponent>() {
    return m_model_component_pool.get();
}

#endif //GAME_WORLD_H