#include "./Application.hpp"

#include <ecs/EventBus.hpp>
#include <ecs/World.hpp>

#include "camera/Camera.hpp"
#include "transform/transform.hpp"

void xc::xcal::handle_event(ecs::EventBus &bus) {}

void xc::xcal::Application::install(ecs::World &world) {
    std::cout << "Installing Application" << std::endl;
    world.add_component<transform::TransformComponent>()
        .add_component<transform::TransformMatrixComponent>();
    if (!world.resource_manager().has<ecs::EventBus>())
        world.add_resource<ecs::EventBus>();
    world.add_resource<camera::ViewConfig>()
        .add_resource<camera::ProjectionConfig>();
    world.add_resource<camera::FpsCameraControler>(
        &world.resource<camera::ViewConfig>(),
        &world.resource<ecs::EventBus>());
}
ecs::World &xc::xcal::Application::run(ecs::World &world) {
    return world.run_system<handle_event>()
        .run_system<camera::update_camera>()
        .run_system<transform::update_transform_matrix>();
}
