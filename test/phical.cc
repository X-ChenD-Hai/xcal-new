#include <gtest/gtest.h>

#include <cstdint>
#include <ecs/command/attach_components.hpp>
#include <ecs/command/command.hpp>
#include <ecs/command/system.hpp>
#include <ecs/command/update_components.hpp>
#include <ecs/command_submit.hpp>
#include <ecs/component_accessor.hpp>
#include <ecs/plugin/core/clock.hpp>
#include <ecs/world.hpp>
#include <functional>
#include <memory>
#include <print>
#include <queue>
#include <vector>
struct Posion {
    float x;
    float y;
};

struct Speed {
    float dx;
    float dy;
};
struct AppHandler {
    uint32_t run_flag;

    static void init(ecs::World& world);
    static void update(ecs::World& world);
};

template <typename Tp>
using less_queue = std::priority_queue<Tp, std::vector<Tp>, std::greater<Tp>>;
void update_phical(ecs::ComponentAccessor acc, ecs::core::Clock& clock) {
    double dt = clock.dt_s();
    acc.each<Posion, Speed>(
        [](Posion& p, Speed& v, double dt) {
            p.x += v.dx * dt;
            p.y += v.dy * dt;
        },
        dt);
};

void print_entity(ecs::ComponentAccessor acc, ecs::core::Clock& clock) {
    acc.each<Posion>(
        [](Posion& pos) { std::println("x:{},y:{}", pos.x, pos.y); });
    clock.run_for(0.1, std::make_unique<ecs::command::System<print_entity>>());
}

void start_a_tick(ecs::core::Clock& c) {
    std::println("tick");
    c.run_for(0.1, std::make_unique<ecs::command::System<start_a_tick>>());
}

void update_app(AppHandler& app, ecs::core::Clock& clock) {
    std::println("runtime: {}", clock.runtime());
    app.run_flag = 0;
}
void AppHandler::init(ecs::World& world) {
    using namespace ecs;
    world.resource<ecs::core::Clock>().run_for(
        1, std::make_unique<
               command::System<[](AppHandler& app) { app.run_flag = 0; }>>());
    world.add_resource<AppHandler>(AppHandler{10});
    world.regist_component<Posion, Speed>();
    auto& submit = world.submit();
    auto e = world.create_entity();
    submit.submit<command::AttachComponents>(e, Posion{0.1, 0.1},
                                             Speed{0.1, 0.1});
}

TEST(Sort, sort) {}

TEST(Phical, phical) {
    ecs::World world;
    world.use_plugin<ecs::core::Clock>()
        .run_system<AppHandler::init>()
        .run_system<print_entity>();
    while (world.resource<AppHandler>().run_flag) {
        world.run_plugin<ecs::core::Clock>();
        world.run_system<update_phical>();
        world.execute_commands();
    }
}
