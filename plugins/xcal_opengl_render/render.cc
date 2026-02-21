#include "./render.hpp"

#include <xc/ecs/world.hpp>
#include <opengl_support/opengl_support.hpp>

#include "./shader.hpp"

xcal_opengl_render::Render* xcal_opengl_render::Render::install(
    ecs::World& world) {
    if (!world.resource_manager().has<opengl_support::OpenGLSupport>()) {
        std::println("OpenGLSupport not installed");
        return nullptr;
    }
    world.add_resource<ShaderTable>();
    return new Render();
}
void xcal_opengl_render::Render::uninstall(ecs::World& world, Render* render) {
    delete render;
}
void xcal_opengl_render::Render::run(ecs::World& world) {
    world.run_system<&ShaderTable::update>();
}
