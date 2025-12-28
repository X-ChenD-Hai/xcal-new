#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <ecs/command/attach_components.hpp>
#include <ecs/command/system.hpp>
#include <ecs/command/update_components.hpp>
#include <ecs/command_submit.hpp>
#include <ecs/world.hpp>
#include <functional>
#include <memory>
#include <print>
#include <queue>
#include <vector>

#include "ecs/command/command.hpp"
#include "ecs/component_accessor.hpp"
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

struct Clock {
    struct TimeOutTask {
        double until;
        mutable std::unique_ptr<ecs::command::Command> command;
        auto operator<=>(const TimeOutTask& t) const noexcept {
            return until <=> t.until;
        }
    };

    struct TickTask {
        std::chrono::duration<double> dur;
        std::unique_ptr<ecs::command::Command> command;
    };
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point last;
    std::chrono::high_resolution_clock::time_point current;
    std::chrono::high_resolution_clock::duration dt;
    double runtime;
    less_queue<TimeOutTask> timeout_tasks;

    void on_timeout(double s, std::unique_ptr<ecs::command::Command>&& cmd) {
        timeout_tasks.emplace(s + runtime, std::move(cmd));
    }
    static void init(ecs::World& word) {
        word.add_resource<Clock>();
        auto& c = word.resource<Clock>();
        c.runtime = 0;
        c.start = c.last = c.current =
            std::chrono::high_resolution_clock::now();

        word.run_system<update>();
    }
    static void update(Clock& clock, ecs::CommandSubmit& submit) {
        clock.last = clock.current;
        clock.current = std::chrono::high_resolution_clock::now();
        clock.dt = clock.current - clock.last;
        clock.runtime =
            std::chrono::duration_cast<std::chrono::duration<double>>(
                clock.current - clock.start)
                .count();
        auto& tasks = clock.timeout_tasks;
        while (!tasks.empty()) {
            auto& t = tasks.top();
            if (t.until > clock.runtime) {
                break;
            }
            submit.submit(std::move(t.command));
            tasks.pop();
        }
    }
};

void update_phical(ecs::ComponentAccessor acc, Clock& clock) {
    double dt =
        std::chrono::duration_cast<std::chrono::duration<double>>(clock.dt)
            .count();
    acc.each<Posion, Speed>(
        [](Posion& p, Speed& v, double dt) {
            p.x += v.dx * dt;
            p.y += v.dy * dt;
        },
        dt);
};

void print_entity(ecs::ComponentAccessor acc, Clock& clock) {
    acc.each<Posion>(
        [](Posion& pos) { std::println("x:{},y:{}", pos.x, pos.y); });
    clock.on_timeout(0.1,
                     std::make_unique<ecs::command::System<print_entity>>());
}

void start_a_tick(Clock& c) {
    std::println("tick");
    c.on_timeout(0.1, std::make_unique<ecs::command::System<start_a_tick>>());
}

void update_app(AppHandler& app, Clock& clock) {
    std::println("runtime: {}", clock.runtime);
    app.run_flag = 0;
}
void AppHandler::init(ecs::World& world) {
    using namespace ecs;
    world.run_system<Clock::init>();
    world.resource<Clock>().on_timeout(
        1, std::make_unique<
               command::System<[](AppHandler& app) { app.run_flag = 0; }>>());
    world.add_resource<AppHandler>(AppHandler{10});
    world.add_component<Posion, Speed>();
    auto& submit = world.submit();
    auto e = world.create_entity();
    submit.submit<command::AttachComponents>(e, Posion{0.1, 0.1},
                                             Speed{0.1, 0.1});
}

TEST(Sort, sort) {}

TEST(Phical, phical) {
    ecs::World world;
    world.run_system<AppHandler::init>().run_system<print_entity>();
    while (world.resource<AppHandler>().run_flag) {
        world.run_system<Clock::update>().run_system<update_phical>();
        world.execute_commands();
    }
}
