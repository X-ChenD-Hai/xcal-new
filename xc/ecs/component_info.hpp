#pragma once
#include <functional>

#include "./entity.hpp"

namespace ecs {
using component_t = uint32_t;
class ComponentInfo {
    friend class World;
    friend class CommandSubmit;
    friend class Querier;
    friend class ComponentAccessor;
    uint32_t pool_index_;
    SparseList<Entity::entity_t, uint32_t, 32> entities_;
    std::function<void(void *)> deleter_;

   public:
    size_t cell_index(Entity e) { return entities_.get_index(e.id()); };
    void add_entity(Entity entity) { entities_.insert(entity.id()); }
    void remove_entity(Entity entity) { entities_.remove(entity.id()); }
    bool has_entity(Entity entity) const {
        return entities_.has_value(entity.id());
    }

    void deallocate(void *ptr) { deleter_(ptr); }
    inline uint32_t pool_index() const noexcept { return pool_index_; }

    ComponentInfo(uint32_t pool_index, std::function<void(void *)> deleter)
        : pool_index_(pool_index), deleter_(deleter) {}
};
}  // namespace ecs