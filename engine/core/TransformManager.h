// TransformManager.h

#ifndef GAME_TRANSFORMMANAGER_H
#define GAME_TRANSFORMMANAGER_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>

#include "Types.h"

class TransformManager {
public:
    TransformManager(size_t capacity = MAX_ENTITIES);

    // API
    void register_entity(Entity entity, Entity parent = 0);
    void remove_entity(Entity entity);

    void set_parent(Entity entity, Entity parent);
    void set_position(Entity entity, const Vec3& Pos);
    void set_rotation(Entity entity, const Quat& rot);
    void set_scale(Entity entity, const Vec3& scale);

    // Getters
    Vec3 get_position(Entity entity) const;
    Quat get_rotation(Entity entity) const;
    Vec3 get_scale(Entity entity) const;

    const Mat4& get_world_transform(Entity entity) const;
    const Mat4& get_local_transform(Entity entity) const;

    // State Sync
    void update_transforms();

private:
    EntityIndex get_index(Entity entity) const;
    void ensure_capacity(size_t min_cap);
    void ensure_schedule();

    // Hierarchy Topology Helpers
    EntityIndex find_free_slot();
    void link_child(EntityIndex parent_idx, EntityIndex child_idx);
    void unlink_child(EntityIndex idx);

    // --- High Level Data (TRS) ---
    std::unordered_map<Entity, EntityIndex> m_entity_to_index;
    std::vector<Entity> m_index_to_entity; // Dense list of active entities (Index -> ID)

    std::vector<Vec3> m_positions;
    std::vector<Quat> m_rotations;
    std::vector<Vec3> m_scales;
    std::vector<uint8_t> m_dirty_flags;

    // --- Low Level Data (Hierarchy & SIMD Transforms) ---
    size_t m_active_count = 0;
    bool m_schedule_dirty = false;

    std::vector<uint8_t> m_is_active;
    std::vector<EntityIndex> m_free_list;
    std::vector<EntityIndex> m_update_schedule; // BFS ordering

    // Topology (Doubly Linked List for tree)
    std::vector<EntityIndex> m_parent;
    std::vector<EntityIndex> m_first_child;
    std::vector<EntityIndex> m_next_sibling;
    std::vector<EntityIndex> m_prev_sibling;

    // Aligned Matrices
    std::vector<Mat4> m_world_mat;
    std::vector<Mat4> m_local_mat;
};

#endif //GAME_TRANSFORMMANAGER_H