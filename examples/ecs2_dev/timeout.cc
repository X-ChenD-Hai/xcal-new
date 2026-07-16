#include <float.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <print>
#include <vector>
#include <xc/ecs2/async_primitives.hpp>
#include <xc/ecs2/scheduler.hpp>
#include <xc/ecs2/types.hpp>
#include <xc/ecs2/utility.hpp>

#include "xc/ecs2/channel.hpp"
#include "xc/ecs2/promise.hpp"
#include "xc/ecs2/structure/consume_token.hpp"
#include "xc/ecs2/structure/ring_buffer.hpp"

using namespace xc::ecs;
using namespace xc::ecs::structure;

System main_sys() {
    RadioStation<int> a{};
    auto sub = a.subscribe();
    auto c = a.publish(11);
    std::println("c: {}", c);
    auto v = co_await sub->listen();
    if (v.has_value()) std::println("v: {}", v.value());
    using namespace std::chrono_literals;
    std::println("main_sys end");
    co_return;
}

int main(int argc, char* argv[]) {
    // test_ring_buffer();
    SystemScheduler scheduler{};
    scheduler.start_workers();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scheduler.add_system(main_sys());
    scheduler.update();
    scheduler.stop_workers();

    return 0;
}
