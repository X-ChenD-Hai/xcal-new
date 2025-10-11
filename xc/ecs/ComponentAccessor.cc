#include "./ComponentAccessor.hpp"

#include "./World.hpp"


void * ecs::ComponentAccessor::data(Entity entity, component_t component_id) {
    auto &info = world_.component_infos_[world_.component2pool_map_.get_index(
        component_id)];
    auto &pool = world_.pools_[info.pool_index_];
    auto cell_index = info.cell_index(entity);
    if (cell_index >= pool.size()) return nullptr;
    return pool[cell_index].get();
};