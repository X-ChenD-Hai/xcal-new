#include <float.h>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <print>
#include <thread>
#include <vector>
#include <xc/ecs2/async_primitives.hpp>
#include <xc/ecs2/scheduler.hpp>
#include <xc/ecs2/types.hpp>
#include <xc/ecs2/utility.hpp>
#include <xc/ecs2/world.hpp>

#include "xc/ecs2/system.hpp"

using namespace xc::ecs;

System async_foreach(int id) {
    std::vector<size_t> tid{};
    auto w = co_await CurrentWorker{};
    auto t = w;
    std::println("fid {} tid {}", id, w->worker_id());
    tid.push_back(w->worker_id());
    co_await Yield{};
    w = co_await CurrentWorker{};
    std::println("fid {} tid {}", id, w->worker_id());
    tid.push_back(w->worker_id());
    co_await Yield{};
    co_await DispatchTo{t};
    w = co_await CurrentWorker{};
    std::println("fid {} tid {}", id, w->worker_id());
    tid.push_back(w->worker_id());
    co_await Yield{};
    w = co_await CurrentWorker{};
    std::println("fid {} tid {}", id, w->worker_id());
    tid.push_back(w->worker_id());
    co_await Yield{};
    co_await DispatchTo{t};
    w = co_await CurrentWorker{};
    std::println("fid {} tid {}", id, w->worker_id());
    tid.push_back(w->worker_id());

    std::println("id:{},tid:{}", id, tid);

    co_return;
}
Future<int> future3(int id) {
    // std::println("----- call future 3");
    // std::println("------- future 3 return");
    co_return id;
}
Future<int> future2(int id) {
    // std::println("----- call future 2");
    auto d = co_await future3(1);
    // std::println("------- future 2 return");
    co_return id + d;
}
Future<int> future1(int id) {
    // std::println("------- call future 1");
    auto d = co_await future2(1);
    // std::println("------- future 1 return");
    co_return id + d;
}
System test_future(int id) {
    utility::ClockRecord r;
    using namespace std::chrono_literals;
    std::println("------ test future");
    auto v = co_await future1(id);
    auto c = 10000;
    r.record();
    for (size_t i = c; i != 0; i--) {
        v += co_await future3(id);
    }
    auto us = r.duration_us();
    std::println("test {} future use {}us per {}us", c, us, us / c);
    co_await Sleep{10ms};
    // v += co_await future2(id) + v;
    std::println("----------v = {}----------", v);
    co_return;
}

System test_sleep() {
    using namespace std::chrono_literals;
    std::println("---start test_sleep");
    co_await Sleep{1s};
    std::println("=====end test_sleep");
    co_return;
}

int main(int argc, char* argv[]) {
    utility::ClockRecord app_record;
    app_record.record();
    auto run_count = 1;
    auto thread_count = 10;
    if (argc > 1) {
        run_count = std::stoi(argv[1]);
    }
    if (argc > 3) {
        thread_count = std::stoi(argv[2]);
    }

    for (size_t i = 0; i < run_count; ++i) {
        utility::ClockRecord app_record;
        app_record.record();
        utility::ClockRecord clock_record;
        SystemScheduler scheduler{};
        clock_record.record();
        scheduler.start_workers(thread_count);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::println("start workers use {} ms", clock_record.duration_ms());
        clock_record.record();
        // scheduler.add_system(test_future(1));
        scheduler.add_system(test_sleep());
        scheduler.add_system(test_sleep());
        scheduler.add_system(test_sleep());
        scheduler.add_system(test_sleep());
        scheduler.add_system(test_sleep());
        scheduler.add_system(test_future(1));
        scheduler.add_system(test_future(2));
        scheduler.add_system(test_future(3));
        scheduler.update();
        auto t = clock_record.duration_ms();
        std::println("run using {} ms", t);
        clock_record.record();
        scheduler.stop_workers();
        std::println("stop using {} s", clock_record.duration());
        std::println("app using {} s", app_record.duration());
    }
    std::println("app using {} ms", app_record.duration_ms());
    std::println("time per run {} ms", app_record.duration_ms() / run_count);
    return 0;
}
