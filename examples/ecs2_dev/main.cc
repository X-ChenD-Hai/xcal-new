#include <float.h>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <print>
#include <thread>

#include "ecs2/async_primitives.hpp"
#include "ecs2/scheduler.hpp"
#include "ecs2/utility.hpp"
#include "ecs2/world.hpp"

using namespace xc::ecs;
System my_task() {
    utility::ClockRecord clock_record;
    clock_record.record();
    using namespace std::chrono_literals;
    for (size_t i = 0; i < 100; ++i) {
    }
    std::println("my_task using {} ms", clock_record.duration_ms());
    co_return;
}

System async_foreach() {
    std::vector<int> vec{};
    vec.resize(144004, 0);
    for (int i = 0; i < vec.size(); ++i) {
        vec[i] = i;
    }
    utility::ClockRecord clock_record;
    clock_record.record();
    using namespace std::chrono_literals;
    co_await AsyncForeach{vec.begin(), vec.end(), [](int& i) {
                              i = 0;
                              std::println("i={}", i);
                          }};
    co_await AsyncForeach{vec.begin(), vec.end(),
                          [](int& i) { assert(i == 0); }};

    co_return;
}

int main(int argc, char* argv[]) {
    utility::ClockRecord clock_record;
    clock_record.record();
    using namespace std::chrono_literals;
    auto n = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 100; ++i) {
        auto vv = i + 1;
    }
    std::println("my_task using {}",
                 std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::high_resolution_clock::now() - n));
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
        {
            utility::ClockRecord app_record;
            app_record.record();
            utility::ClockRecord clock_record;
            SystemScheduler scheduler{};
            clock_record.record();
            scheduler.start_workers(thread_count);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            std::println("start workers use {} ms", clock_record.duration_ms());
            clock_record.record();
            scheduler.add_system(async_foreach);
            scheduler.add_system(my_task);
            scheduler.add_system(my_task);
            scheduler.add_system(my_task);
            scheduler.update();
            auto t = clock_record.duration_ms();
            std::println("run using {} ms", t);
            clock_record.record();
            scheduler.stop_workers();
            std::println("stop using {} s", clock_record.duration());
            std::println("app using {} s", app_record.duration());
        }
    }
    std::println("app using {} ms", app_record.duration_ms());
    std::println("time per run {} ms", app_record.duration_ms() / run_count);
    // assert(task_finish_count.load() == task_count.load());
    return 0;
}
