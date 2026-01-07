#include "./render.hpp"
xcal_opengl_render::Render* xcal_opengl_render::Render::install(
    ecs::World& world) {
    return new Render();
}
void xcal_opengl_render::Render::uninstall(ecs::World& world, Render* render) {
    delete render;
}
void xcal_opengl_render::Render::run(ecs::World& world) {}
