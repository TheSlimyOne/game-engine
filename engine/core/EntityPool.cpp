#include "EntityPool.h"
#include <cassert>

EntityPool::EntityPool(size_t max_entities)
    :   m_next(1),
        m_capacity(max_entities)
{}

Entity EntityPool::create() {
    if (!m_free.empty()) {
        Entity id = m_free.back();
        m_free.pop_back();
        return id;
    }

    assert(m_next < m_capacity && "Exceeded maximum number of entities!");
    return static_cast<Entity>(m_next++);
}

void EntityPool::destroy(Entity entity) {
    assert(entity < m_capacity && "Entity out of bounds!");
    m_free.push_back(entity);
}



