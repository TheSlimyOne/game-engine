// TransformManager.cpp

#include "TransformManager.h"

TransformManager::TransformManager(size_t capacity) {
    ensure_capacity(capacity);

    // Initialize Internal Root (Index 0)
    // This represents "World Space". It has no Entity ID associated with it.
    m_active_count = 1;
    m_is_active[0] = 1;
    m_index_to_entity.push_back(0); // Index 0 maps to Entity 0 (Null/Root)

    // Root Data (Identity)
    m_positions[0] = Vec3(0.0f);
    m_rotations[0] = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_scales[0]    = Vec3(1.0f);
    m_dirty_flags[0] = 0;

    m_local_mat[0] = Mat4::identity();
    m_world_mat[0] = Mat4::identity();
    m_update_schedule.push_back(0);

    for (EntityIndex i = static_cast<EntityIndex>(capacity) - 1; i >= 1; --i) {
        m_free_list.push_back(i);
    }
}

void TransformManager::ensure_capacity(size_t cap) {
    if (m_positions.size() < cap) {
        m_positions.resize(cap);
        m_rotations.resize(cap);
        m_scales.resize(cap);
        m_dirty_flags.resize(cap);

        m_is_active.resize(cap, 0);
        m_parent.resize(cap, NO_ENTITY);
        m_first_child.resize(cap, NO_ENTITY);
        m_next_sibling.resize(cap, NO_ENTITY);
        m_prev_sibling.resize(cap, NO_ENTITY);

        // This will call Mat4() default constructor for new elements
        m_local_mat.resize(cap);
        m_world_mat.resize(cap);

        m_index_to_entity.reserve(cap);
    }
}

EntityIndex TransformManager::get_index(Entity entity) const {
    if (entity == 0) return NO_ENTITY; // Entity 0 is strictly invalid/null
    auto it = m_entity_to_index.find(entity);
    // If not found, return NO_ENTITY.
    // Do NOT return 0, or we risk modifying the World Root by accident.
    return (it != m_entity_to_index.end()) ? it->second : NO_ENTITY;
}

EntityIndex TransformManager::find_free_slot() {
    if (m_free_list.empty()) {
        ensure_capacity(m_positions.size() * 2);
        for (size_t i = m_positions.size() - 1; i > m_active_count; --i) {
            m_free_list.push_back(static_cast<EntityIndex>(i));
        }
    }
    EntityIndex idx = m_free_list.back();
    m_free_list.pop_back();
    return idx;
}

void TransformManager::link_child(EntityIndex p_idx, EntityIndex c_idx) {
    m_parent[c_idx] = p_idx;
    EntityIndex old_first = m_first_child[p_idx];

    m_next_sibling[c_idx] = old_first;
    m_prev_sibling[c_idx] = NO_ENTITY;

    if (old_first != NO_ENTITY) m_prev_sibling[old_first] = c_idx;
    m_first_child[p_idx] = c_idx;
}

void TransformManager::unlink_child(EntityIndex idx) {
    EntityIndex p_idx = m_parent[idx];
    if (p_idx == NO_ENTITY) return;

    EntityIndex prev = m_prev_sibling[idx];
    EntityIndex next = m_next_sibling[idx];

    if (next != NO_ENTITY) m_prev_sibling[next] = prev;
    if (prev != NO_ENTITY) m_next_sibling[prev] = next;
    else m_first_child[p_idx] = next;

    m_parent[idx] = NO_ENTITY;
    m_next_sibling[idx] = NO_ENTITY;
    m_prev_sibling[idx] = NO_ENTITY;
}

void TransformManager::register_entity(Entity entity, Entity parent) {
    if (entity == 0) return; // Cannot register Null Entity
    if (m_entity_to_index.find(entity) != m_entity_to_index.end()) return;

    // Resolve Parent:
    // If parent is 0 (Null), attach to Internal Index 0 (World Root).
    // If parent is Valid but not found, fallback to World Root.
    EntityIndex parent_idx = 0;
    if (parent > 0) {
        EntityIndex found = get_index(parent);
        if (found != NO_ENTITY) parent_idx = found;
    }

    EntityIndex idx = find_free_slot();

    m_entity_to_index[entity] = idx;
    if (idx >= m_index_to_entity.size()) m_index_to_entity.resize(idx + 1);
    m_index_to_entity[idx] = entity;

    m_is_active[idx] = 1;
    m_local_mat[idx] = Mat4::identity();
    m_world_mat[idx] = Mat4::identity();
    m_first_child[idx] = NO_ENTITY;

    link_child(parent_idx, idx);
    m_schedule_dirty = true;
    m_active_count++;

    m_positions[idx] = Vec3(0.0f);
    m_rotations[idx] = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_scales[idx]    = Vec3(1.0f);
    m_dirty_flags[idx] = 1;
}

void TransformManager::remove_entity(Entity entity) {
    EntityIndex idx = get_index(entity);
    if (idx == NO_ENTITY || idx == 0) return; // Cannot remove Root

    unlink_child(idx);

    m_is_active[idx] = 0;
    m_free_list.push_back(idx);
    m_active_count--;

    m_entity_to_index.erase(entity);
    m_schedule_dirty = true;
}

void TransformManager::set_parent(Entity entity, Entity parent) {
    EntityIndex e_idx = get_index(entity);
    if (e_idx == NO_ENTITY || e_idx == 0) return;

    EntityIndex p_idx = 0; // Default to Root
    if (parent > 0) {
        EntityIndex found = get_index(parent);
        if (found != NO_ENTITY) p_idx = found;
    }

    if (m_parent[e_idx] == p_idx) return;

    unlink_child(e_idx);
    link_child(p_idx, e_idx);
    m_schedule_dirty = true;
}



// --- Setters ---
// Guard: Check for NO_ENTITY and Index 0.
// We should not allow setting properties on the internal World Root.
void TransformManager::set_position(Entity entity, const Vec3& pos) {
    EntityIndex idx = get_index(entity);
    if (idx != NO_ENTITY && idx != 0) { m_positions[idx] = pos; m_dirty_flags[idx] = 1; }
}
void TransformManager::set_rotation(Entity entity, const Quat& rot) {
    EntityIndex idx = get_index(entity);
    if (idx != NO_ENTITY && idx != 0) { m_rotations[idx] = rot; m_dirty_flags[idx] = 1; }
}
void TransformManager::set_scale(Entity entity, const Vec3& scale) {
    EntityIndex idx = get_index(entity);
    if (idx != NO_ENTITY && idx != 0) { m_scales[idx] = scale; m_dirty_flags[idx] = 1; }
}

// --- Getters ---
// Guard: If entity not found, return default (0,0,0) or Identity.
// Reading from Index 0 (Root) is safe and valid (it returns 0,0,0).
Vec3 TransformManager::get_position(Entity entity) const {
    EntityIndex idx = get_index(entity);
    return (idx != NO_ENTITY) ? m_positions[idx] : Vec3(0.0f);
}
Quat TransformManager::get_rotation(Entity entity) const {
    EntityIndex idx = get_index(entity);
    return (idx != NO_ENTITY) ? m_rotations[idx] : Quat(1.0f, 0.0f, 0.0f, 0.0f);
}
Vec3 TransformManager::get_scale(Entity entity) const {
    EntityIndex idx = get_index(entity);
    return (idx != NO_ENTITY) ? m_scales[idx] : Vec3(1.0f);
}

const Mat4& TransformManager::get_world_transform(Entity entity) const {
    EntityIndex idx = get_index(entity);
    return (idx != NO_ENTITY) ? m_world_mat[idx] : m_world_mat[0];
}
const Mat4& TransformManager::get_local_transform(Entity entity) const {
    EntityIndex idx = get_index(entity);
    return (idx != NO_ENTITY) ? m_local_mat[idx] : m_local_mat[0];
}

void TransformManager::ensure_schedule() {
    if (!m_schedule_dirty) return;

    m_update_schedule.clear();
    static std::vector<EntityIndex> queue;
    if (queue.capacity() < m_positions.size()) queue.reserve(m_positions.size());
    queue.clear();

    queue.push_back(0);
    m_update_schedule.push_back(0);

    size_t head = 0;
    while(head < queue.size()) {
        EntityIndex p = queue[head++];
        EntityIndex child = m_first_child[p];
        while (child != NO_ENTITY) {
            if (m_is_active[child]) {
                m_update_schedule.push_back(child);
                queue.push_back(child);
            }
            child = m_next_sibling[child];
        }
    }
    m_schedule_dirty = false;
}

void TransformManager::update_transforms() {
    ensure_schedule();

    size_t count = m_update_schedule.size();
    const EntityIndex* schedule_ptr = m_update_schedule.data();

    // 1. Compose Local Matrices
    for (size_t i = 0; i < count; ++i) {
        EntityIndex idx = schedule_ptr[i];
        if (m_dirty_flags[idx]) {
            m_local_mat[idx] = Mat4(m_positions[idx], m_rotations[idx], m_scales[idx]);
            m_dirty_flags[idx] = 0;
        }
    }

    // 2. Solve Hierarchy
    Mat4* __restrict world_ptr = m_world_mat.data();
    const Mat4* __restrict local_ptr = m_local_mat.data();
    const EntityIndex* __restrict parent_ptr = m_parent.data();

    for (size_t i = 1; i < count; ++i) {
        EntityIndex idx = schedule_ptr[i];
        EntityIndex p = parent_ptr[idx];
        world_ptr[idx] = Mat4::mul(world_ptr[p], local_ptr[idx]);
    }
}