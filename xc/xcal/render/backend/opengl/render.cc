#include "./render.hpp"

#include <ecs/world.hpp>
#include <xcal/transform/transform.hpp>

#include "./event.hpp"
#include "./mesh.hpp"
#include "./shader.hpp"

xc::xcal::render::opengl::Render *xc::xcal::render::opengl::Render::install(
    ecs::World &world) {
    if (!world.resource_manager().has<ecs::ResourceTable>())
        world.add_resource<ecs::ResourceTable>();
    world.add_component<SingleColorShaderComponent>()
        .add_component<LinearGradientShaderComponent>()
        .add_component<RadialGradientShaderComponent>()
        .add_component<VertexColorShaderComponent>()
        .add_component<Texture2dShaderComponent>()
        .add_component<Texture3dShaderComponent>()
        .add_component<MeshComponent>();
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
    auto mesh_comp = mesh.mesh_component();
    transform_component.state = xc::xcal::transform::TransformState::Dirty;
    std::visit(
        [&](auto &&shader) {
            auto entity = world_.create_entity();
            world_.submit().submit<ecs::AttachComponents>(
                entity, transform_component, mesh_comp, shader,
                xc::xcal::transform::TransformMatrixComponent{});
        },
        mesh.shader_program);
}
