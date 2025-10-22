#include <ui/event.h>

#include <ecs/command_submit.hpp>
#include <ecs/component_accessor.hpp>
#include <ecs/querier.hpp>
#include <ecs/world.hpp>
#include <event/event.hpp>
#include <id_generator.hpp>
#include <print>
#include <sparse_list.hpp>
#include <xc_assert.hpp>

using Loop = std::unique_ptr<EventLoop>;
void send_event(Loop& eventloop) {
    eventloop->publish(std::make_unique<Event>(EventType::TimeOut, nullptr));
}
void linster_event(ecs::World& world, Loop& eventloop) {
    if (!eventloop->events().empty()) {
        world.quit();
        std::println("quit");
    }
}

int main() {
    std::println("ecs_event");
    auto eventloop = std::make_unique<EventLoop>();
    eventloop->make_global();

    ecs::World world;
    world.add_resource<Loop>(std::move(eventloop))
        .add_system<send_event>()
        .add_system<linster_event>();
    while (!world.should_quit()) {
        world.update();
    }
    return 0;
}
