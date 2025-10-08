#include <ui/event.h>

#include <IdGenerator.hpp>
#include <SparseList.hpp>
#include <ecs/CommandSubmit.hpp>
#include <ecs/ComponentAccessor.hpp>
#include <ecs/Querier.hpp>
#include <ecs/World.hpp>
#include <event/event.hpp>
#include <print>
#include <xc_assert.hpp>


using Loop = std::unique_ptr<EventLoop>;
void send_event(Loop& eventloop) {
    eventloop->publish(std::make_unique<Event>(EventType::TimeOut, nullptr));
}
void linster_event(World& world, Loop& eventloop) {
    if (!eventloop->events().empty()) {
        world.quit();
        std::println("quit");
    }
}

int main() {
    std::println("ecs_event");
    auto eventloop = std::make_unique<EventLoop>();
    eventloop->make_global();

    World world;
    world.add_resource<Loop>(std::move(eventloop))
        ->add_system<send_event>()
        ->add_system<linster_event>();
    while (!world.should_quit()) {
        world.update();
    }
    return 0;
}
