#pragma once
#include <xc/ecs/tyoes.hpp>

namespace xcal_opengl_render {
class Render {
    friend class ::ecs::World;
    static Render* install(ecs::World& world);
    void run(ecs::World& world);
    static void uninstall(ecs::World& world, Render* render);
};
}  // namespace xcal_opengl_render