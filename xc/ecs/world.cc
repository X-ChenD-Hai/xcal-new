#include "./world.hpp"

#include "./command_submit.hpp"
#include "./component_accessor.hpp"
#include "./querier.hpp"

ecs::Querier ecs::World::querier() const noexcept { return {*this, {}}; }
ecs::ComponentAccessor ecs::World::accessor() noexcept { return {*this}; }

ecs::CommandSubmit& ecs::World::submit() { return command_submit_; }

ecs::World::~World() {}
ecs::World::World() {};
void ecs::World::attach_component(component_t comp, Entity e, void* data) {
    auto& info = component_info(comp);
    XC_ASSERT(!info.has_entity(e));
    auto pool_index = info.pool_index();
    pools_[pool_index].emplace_back(data, info.deleter_);
    info.add_entity(e);
}
void ecs::World::detach_component(component_t comp, Entity e) {
    auto& info = component_info(comp);
    XC_ASSERT(info.has_entity(e));
    pools_[info.pool_index()][info.cell_index(e)].reset();
    info.remove_entity(e);
}
void ecs::World::modify_component(component_t comp, Entity e, void* data) {
    auto& info = component_info(comp);
    XC_ASSERT(info.has_entity(e));
    pools_[info.pool_index()][info.cell_index(e)] = Cell_(data, info.deleter_);
}
