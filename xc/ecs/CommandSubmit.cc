#include "./CommandSubmit.hpp"

#include "./World.hpp"

void ecs::CommandSubmit::execute_then_clear(World &world) {
    for (auto &command : commands_) {
        command->execute(world);
    }
    commands_.clear();
}
void ecs::CreateEntityCommand::execute(World &world) {
    auto entity = world.create_entity();
    for (auto &component : components_) {
        world.attach_component(component.first, entity, component.second);
    }
};