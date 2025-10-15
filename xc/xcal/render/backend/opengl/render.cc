#include "./Render.hpp"

#include <ecs/World.hpp>
#include <xcal/transform/transform.hpp>

#include "./Event.hpp"
#include "./Mesh.hpp"
#include "./Shader.hpp"

ecs::World &xc::xcal::render::opengl::Render::install(ecs::World &world) {
    std::println("Installing render system");
    return world.add_component<ShaderComponent>()
        .add_component<MeshComponent>();
}
ecs::World &xc::xcal::render::opengl::Render::run(ecs::World &world) {
    return world.run_system<handle_event>().run_system<render_mesh>();
}
