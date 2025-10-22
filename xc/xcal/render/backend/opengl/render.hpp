#pragma once
#include <ecs/querier.hpp>
#include <xcal/transform/transform.hpp>

#include "./mesh/trangle.hpp"

namespace ecs {
class World;
}

namespace xc::xcal::render::opengl {
class Render {
    friend class ecs::World;
    ecs::World &world_;
    xc::xcal::transform::TransformComponent transform_component;

   protected:
    static Render *install(ecs::World &world);
    static void uninstall(ecs::World &world, Render *render);

    static ecs::World &run(ecs::World &world);

    Render(ecs::World &world) : world_(world) {}

   public:
    void add_mesh(const Mesh &mesh,
                  xc::xcal::transform::TransformComponent transform_component =
                      xc::xcal::transform::TransformComponent{});
};
}  // namespace xc::xcal::render::opengl
