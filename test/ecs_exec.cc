#include <gtest/gtest.h>

#include <print>
#include <xc/ecs_executor/executor.hpp>

#include "xc/async/scheduler.hpp"
#include "xc/async/system.hpp"
#include "xc/ecs2/component.hpp"
#include "xc/ecs2/system.hpp"

using namespace xc::ecs_executor;
using namespace xc::ecs;
TEST(ECS_exec, exec) {
    Schedule s{};
    s.registry().regist<int, double, float, long>();
    s.add_system<System<ComponentQuery<int>>>([](ComponentQuery<int>& q) {
        std::println("---------- sys_run ---------");
        q.each([](auto& i) { std::println("{}", i); });
    });
    auto sys = [&]() -> xc::async::System {
        co_await xc::ecs_executor::update(s, 4);
    }();

    auto sc = xc::async::SystemScheduler{};
    sc.start_workers(4);
    sc.add_system(std::move(sys));
    sc.update();
    sc.stop_workers();
}
