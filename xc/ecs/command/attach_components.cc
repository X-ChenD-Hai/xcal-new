#include "./attach_components.hpp"

#include <ecs/world.hpp>
void ecs::command::AttachComponents::execute(World& world) const {
    for (auto& component : components_) {
        world.attach_component(component.first, entity_, component.second);
    }
};
