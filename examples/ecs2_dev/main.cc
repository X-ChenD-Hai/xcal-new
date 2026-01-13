#include <float.h>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <print>
#include <thread>
#include <vector>

#include "ecs2/async_primitives.hpp"
#include "ecs2/scheduler.hpp"
#include "ecs2/types.hpp"
#include "ecs2/utility.hpp"
#include "ecs2/world.hpp"

using namespace xc::ecs;

System async_foreach() {
    std::vector<int> vec{};
    vec.resize(10004, 0);
    for (int i = 0; i < vec.size(); ++i) {
        vec[i] = i;
    }
    utility::ClockRecord clock_record;
    clock_record.record();
    using namespace std::chrono_literals;

    auto f1 = Future{[]() {
        for (size_t i = 0; i < 100; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::println("a={}", i);
        }
        return 11;
    }};
    auto f2 = Future{[]() {
        for (size_t j = 0; j < 100; ++j) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::println("b={}", j);
        }
        return 12;
    }};
    auto f3 = Future{[]() {
        for (size_t j = 0; j < 100; ++j) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::println("c={}", j);
        }
        return 12;
    }};


    auto foreach_t = AsyncForeach{vec.begin(), vec.end(), [](int& i) {
                                      i *= 2;
                                      std::println("i={}", i);
                                  }};
    auto [a, b, c] = (co_await (f1 && f2 && f3)).values();
    std::println("a {} b {}", a.value(), b.value());
    co_await foreach_t;
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
    return 0;
}
