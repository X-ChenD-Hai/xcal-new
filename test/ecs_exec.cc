#include <gtest/gtest.h>

#include <print>
#include <type_traits>
#include <xc/ecs_executor/executor.hpp>

#include "ecs2/markers.hpp"
#include "xc/async/scheduler.hpp"
#include "xc/async/system.hpp"
#include "xc/ecs2/component.hpp"
#include "xc/ecs2/system.hpp"

using namespace xc::ecs_executor;
using namespace xc::ecs;

class Msys : public System<ComponentQuery<int>, Derived<Msys>> {
   public:
    void execute() {
        std::println("---------- sys_run ---------");
        std::println("{}", query().query().size());
    }
};

TEST(ECS_exec, exec) {
    constexpr bool s11 = has_execute_v<Msys>;

    Schedule s{};
    s.registry().regist<int, double, float, long>();
    s.add_system<Msys>();

    auto sys = [&]() -> xc::async::System {
        co_await xc::ecs_executor::update(s, 4);
    }();

    auto sc = xc::async::SystemScheduler{};
    sc.start_workers(4);
    sc.add_system(std::move(sys));
    sc.update();
    sc.stop_workers();
}
