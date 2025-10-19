#pragma once
namespace ecs {
class World;
class EventBus;
};

namespace xc::xcal {

class Application {
   public:
    static void install(ecs::World &world);

    static ecs::World &run(ecs::World &world);
};
void handle_event(ecs::EventBus &bus);
}  // namespace xc::xcal
