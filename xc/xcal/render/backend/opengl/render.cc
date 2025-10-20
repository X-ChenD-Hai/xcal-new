#include "./Render.hpp"

#include <ecs/World.hpp>
#include <xcal/transform/transform.hpp>

#include "./Event.hpp"
#include "./Mesh.hpp"
#include "./Shader.hpp"

xc::xcal::render::opengl::Render *xc::xcal::render::opengl::Render::install(
    ecs::World &world) {
    std::println("Installing render system");
    world.add_component<ShaderComponent>().add_component<MeshComponent>();
    if (!world.resource_manager().has<ecs::ResourceTable>())
        world.add_resource<ecs::ResourceTable>();
    return new Render(world);
}
ecs::World &xc::xcal::render::opengl::Render::run(ecs::World &world) {
    return world.run_system<handle_event>().run_system<render_mesh>();
}
void xc::xcal::render::opengl::Render::uninstall(ecs::World &world,
                                                 Render *render) {
    delete render;
}
void xc::xcal::render::opengl::Render::add_mesh(
    const Mesh &mesh,
    xc::xcal::transform::TransformComponent transform_component) {
    auto shdaer =
        world_.resource<ecs::ResourceTable>().create_or_get<Shader, Trangle>(
            "./res/line.vs", "./res/line.fs");

    auto shader_component = ShaderComponent{.program_id = shdaer->program};
    auto mesh_comp = mesh.mesh_component();
    std::println("Adding mesh {}",mesh_comp.type);
    transform_component.state = xc::xcal::transform::TransformState::Dirty;
    world_.submit().create_entity(
        transform_component, shader_component, mesh_comp,
        xc::xcal::transform::TransformMatrixComponent{});
}
