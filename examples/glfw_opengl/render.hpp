#pragma once

#include <memory>

#include <xc/ecs/event_bus.hpp>
#include <xc/ecs/world.hpp>

namespace app {
struct RenderHandle;
class Renderer {
    friend class ::ecs::World;

   public:
    static void init(ecs::World& world);
    void run(ecs::World& world, ecs::EventBus& event_bus);

   public:
    Renderer(std::unique_ptr<RenderHandle> render_handle);

   private:
    std::unique_ptr<RenderHandle> handle_;
};
}  // namespace app