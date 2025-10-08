#pragma once
#include "./Entity.hpp"
#include "./ComponentInfo.hpp"
class World;
class Entity;
class ComponentAccessor {
    World &world_;

   public:
    ComponentAccessor(World &world) : world_(world) {}
    template <typename Component>
    Component *data(Entity entity);
    void *data(Entity entity, component_t component_id);
};


template <typename Component>
Component *ComponentAccessor::data(Entity entity) {
    return static_cast<Component *>(data(entity,ComponentIdGenerator<Component>::get()));
}
