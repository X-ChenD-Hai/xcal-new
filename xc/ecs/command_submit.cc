#include "./command_submit.hpp"

#include "./world.hpp"

void ecs::CommandSubmit::execute_then_clear(World &world) {
    for (auto &commands : commands_) {
        for (auto &command : commands) command->execute(world);
        commands.clear();
    }
}
void ecs::AttachComponents::execute(World &world) const {
    for (auto &component : components_) {
        world.attach_component(component.first, entity_, component.second);
    }
};