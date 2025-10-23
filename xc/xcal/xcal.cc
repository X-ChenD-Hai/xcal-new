#include "./xcal.hpp"

#include <ecs/event_bus.hpp>
#include <ecs/world.hpp>

#include "./animation/animation.hpp"
#include "./camera/camera.hpp"
#include "./transform/transform.hpp"

void xc::xcal::handle_event(ecs::EventBus &bus) {}

void xc::xcal::Xcal::install(ecs::World &world) {
    if (!world.resource_manager().has<ecs::EventBus>())
        world.add_resource<ecs::EventBus>();
    camera::details::setup(world);
    transform::details::setup(world);
    animation::details::setup(world);
}
ecs::World &xc::xcal::Xcal::run(ecs::World &world) {
    return world.run_system<handle_event>()
        .run_system<animation::details::run>()
        .run_system<camera::details::run>()
        .run_system<transform::details::run>();
}
