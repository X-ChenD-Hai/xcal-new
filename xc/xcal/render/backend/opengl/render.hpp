#pragma once
#include <ecs/Querier.hpp>
namespace ecs {
class World;
}

namespace xc::xcal::render::opengl {
class Render {
   public:
    static ecs::World &install(ecs::World &world);

    static ecs::World &run(ecs::World &world);
};
}  // namespace xc::xcal::render::opengl
