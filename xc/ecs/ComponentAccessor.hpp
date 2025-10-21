#pragma once
#include "./ComponentInfo.hpp"
#include "./Entity.hpp"
namespace ecs {
class World;
class Entity;
class ComponentAccessor {
    World &world_;

   public:
    ComponentAccessor(World &world) : world_(world) {}
    template <typename Component>
    Component *data(Entity entity);
    template <typename... Component>
        requires(sizeof...(Component) > 1)
    std::tuple<Component *...> data(Entity entity);
    void *data(Entity entity, component_t component_id);
    template <typename... Component, typename Fn,typename ...Args>
        requires std::is_invocable_v<Fn, Component& ..., Args...>
    void each(Fn &&fn, Args &&...args);
};

template <typename Component>
Component *ComponentAccessor::data(Entity entity) {
    return static_cast<Component *>(
        data(entity, ComponentIdGenerator<Component>::get()));
}
template <typename... Component>
    requires(sizeof...(Component) > 1)
inline std::tuple<Component *...> ComponentAccessor::data(Entity entity) {
    return std::make_tuple(data<Component>(entity)...);
}

}  // namespace ecs