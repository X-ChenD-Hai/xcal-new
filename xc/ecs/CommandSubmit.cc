#include "./CommandSubmit.hpp"

#include "./World.hpp"

void ecs::CommandSubmit::execute_then_clear(World &world) {
    for (auto &commands : commands_) {
        for (auto &command : commands) command->execute(world);
        commands.clear();
    }
}
void ecs::CreateEntity::execute(World &world) const {
    auto entity = world.create_entity();
    for (auto &component : components_) {
        world.attach_component(component.first, entity, component.second);
    }
    on_created_(entity);
};