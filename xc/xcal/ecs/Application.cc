#include "./Application.hpp"

#include <ecs/EventBus.hpp>
#include <ecs/World.hpp>

#include "transform/transform.hpp"


void handle_event(ecs::EventBus &bus) {}

ecs::World &xc::xcal::Application::install(ecs::World &world) {
    std::cout << "Installing Application" << std::endl;
    return world.add_component<transform::TransformComponent>()
        .add_component<transform::TransformMatrixComponent>();
}
ecs::World &xc::xcal::Application::run(ecs::World &world) {
    std::cout << "Running Application" << std::endl;
    return world.run_system<handle_event>()
        .run_system<transform::update_transform_matrix>();
}
