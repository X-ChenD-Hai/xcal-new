#include "./detach_components.hpp"

#include <ecs/world.hpp>
void ecs::command::DetachComponents::execute(World& world) const {
    for (auto& component : components_) {
        world.detach_component(component, entity_);
    }
}
