#include "./update_components.hpp"

#include <ecs/world.hpp>
void ecs::command::UpdateComponents::execute(World& world) const {
    for (auto& component : components_) {
        world.modify_component(component.first, entity_, component.second);
    }
};
