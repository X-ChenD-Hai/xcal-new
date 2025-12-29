#include <gtest/gtest.h>

#include <ecs/plugin/core/clock.hpp>
#include <ecs/world.hpp>
#include <memory>
#include <print>

#include "ecs/command/system.hpp"
struct App {
    bool running;
};

void print_tick(ecs::core::Clock& clock) {
    std::println("tick");
    clock.run_for(0.1, std::make_unique<ecs::command::System<print_tick>>());
}

TEST(Clock, clock) {
    using namespace ecs;
    World world;
    world.add_resource<App>(true);
    world.use_plugin<core::Clock>();
    world.plugin<core::Clock>().run_for(
        1, std::make_unique<
               ecs::command::System<[](App& app) { app.running = false; }>>());
    world.run_system<print_tick>();
    while (world.resource<App>().running) {
        world.run_plugin<core::Clock>();
        world.execute_commands();
    }
}
