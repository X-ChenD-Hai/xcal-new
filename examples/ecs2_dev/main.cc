#include <float.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <future>
#include <optional>
#include <print>
#include <ranges>
#include <thread>
#include <vector>
#include <xc/ecs2/async_primitives.hpp>
#include <xc/ecs2/scheduler.hpp>
#include <xc/ecs2/types.hpp>
#include <xc/ecs2/utility.hpp>

#include "xc/ecs2/structure/ring_buffer.hpp"
#include "xc/ecs2/system.hpp"

using namespace xc::ecs;
using namespace xc::ecs::structure;

System test_dispatch(int id) {
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
    co_await Yield{};
    w = co_await CurrentWorker{};
    std::println("fid {} tid {}", id, w->worker_id());
    tid.push_back(w->worker_id());
    {
        auto sc = co_await ScopeBind{t};
        std::println("fid {} on scope bind tid {}", id, w->worker_id());
        co_await Yield{};
        w = co_await CurrentWorker{};
        std::println("fid {} tid {}", id, w->worker_id());
        tid.push_back(w->worker_id());
        co_await Yield{};
        w = co_await CurrentWorker{};
        std::println("fid {} tid {}", id, w->worker_id());
        tid.push_back(w->worker_id());
        co_await Yield{};
        w = co_await CurrentWorker{};
        std::println("fid {} tid {}", id, w->worker_id());
        tid.push_back(w->worker_id());
    }
    co_await Yield{};
    w = co_await CurrentWorker{};
    std::println("fid {} tid {}", id, w->worker_id());
    tid.push_back(w->worker_id());

    std::println("id:{},tid:{}", id, tid);

    auto workers = co_await Workers{};
    std::println("workers {}", workers | std::views::transform([](auto w) {
                                   return w->worker_id();
                               }));
    co_return;
}
Future<int> future3(int id) {
    std::println("----- call future 3");
    std::println("------- future 3 return");
    co_return id;
}
Future<int> future2(int id) {
    using namespace std::chrono_literals;
    std::println("----- call future 2");
    co_await Sleep{1ms * (std::rand() % 10)};
    auto d = co_await future3(1);
    std::println("------- future 2 return");
    co_return id + d;
}
Future<int> future1(int id) {
    std::println("------- call future 1");
    auto d = co_await future2(1);
    std::println("------- future 1 return");
    co_return id + d;
}
System test_future(int id) {
    utility::ClockRecord r;
    using namespace std::chrono_literals;
    std::println("------ test future");
    auto v = co_await future1(id);
    auto c = 10;
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
System test_join_func_future() {
    std::println("test_join_func_future");
    Future f1{[]() { return 1; }};
    Future f2{[]() { return 2; }};
    Future f3{[]() { return 3; }};
    Future f4{[]() { return 4; }};
    auto v1 = co_await f1;
    auto v2 = co_await f2;
    auto v3 = co_await f3;
    auto v4 = co_await f4;
    std::println("v1 = {}, v2 = {}", v1, v2);
    Future f11{[]() { return 11; }};
    Future f12{[]() { return 12; }};
    Future f13{[]() { return 13; }};
    Future f14{[]() { return 14; }};
    auto [v11, v12, v13, v14] = co_await (f11 && f12 && f13 && f14);
    std::println("v111 = {}, v12 = {}, v13 = {}, v14 = {}", v11, v12, v13, v14);

    co_return;
}
System test_join_async_and_func_future() {
    std::println("test_join_async_func_future");
    auto [v11, v12, v13, v14] =
        co_await (future3(3) && future2(2) && future1(1) && future1(3));
    std::println("v111 = {}, v12 = {}, v13 = {}, v14 = {}", v11, v12, v13, v14);
    Future ff12{[]() { return 12; }};
    Future ff13{[]() { return 13; }};
    auto [f11, f12, f13, f14] =
        co_await (future3(3) && ff13 && future1(1) && ff12);
    std::println("f111 = {}, f12 = {}, f13 = {}, f14 = {}", v11, v12, v13, v14);

    co_return;
}

System test_when_all_func_future() {
    std::println("test_all_done");
    std::vector<Future<int, true>> f1{};
    f1.emplace_back([]() { return 1; });
    f1.emplace_back([]() { return 1; });
    f1.emplace_back([]() { return 1; });
    f1.emplace_back([]() { return 1; });

    std::println("wait all done");
    auto n = WhenAll{f1};
    std::println("c = {}", f1.empty());
    auto v = co_await n;
    std::println("n = {}", v);

    co_return;
}
System test_when_all_async_future() {
    using namespace std::chrono_literals;
    std::println("test_when_all_async_future");
    std::vector<Future<int>> f1{};
    for (size_t i = 0; i < (std::rand() % 10); i++) {
        f1.emplace_back([]() -> Future<int> {
            auto x = std::rand() % 1000;
            co_await Sleep{1us * x};
            co_return x;
        }());
    }
    auto v = co_await WhenAll{std::move(f1)};
    std::println("v = {}", v);
    co_return;
}
using channel_t = Channel<int, 16>;
Future<int> producer(channel_t& ch) {
    for (size_t i = 0; i < 10; i++) {
        std::println("send {}", i);
        co_await ch.send(i);
        std::println("send done {}", i);
    }

    co_return 0;
}
Future<int> consumer(channel_t& ch) {
    for (size_t i = 0; i < 10; i++) {
        auto v = co_await ch.recv();
        std::println("v = {}", v);
    }
    co_return 0;
}

System test_channel_future_when_all() {
    std::println("test_channel");
    channel_t ch;
    auto v = std::vector<Future<int>>{};
    v.emplace_back(producer(ch));
    v.emplace_back(consumer(ch));
    auto q = co_await WhenAll{v};
    std::println("q = {}", q);
    co_return;
}
System test_channel_future_join() {
    std::println("test_channel");
    channel_t ch;
    auto q = co_await (producer(ch) && consumer(ch));
    std::println("q = {}", q);
    co_return;
}

void run_system() {
    utility::ClockRecord app_record;
    app_record.record();
    auto run_count = 1;
    auto thread_count = 1.5 * std::thread::hardware_concurrency();
    // auto thread_count = 4;

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
        const auto test_count = 10000;
        // const auto test_count = 5;
        try {
            for (size_t i = 0; i < test_count; ++i) {
                // ok
                scheduler.add_system(test_dispatch(1));
                // ok
                scheduler.add_system(test_future(2));
                // ok
                scheduler.add_system(test_sleep());
                // ok
                scheduler.add_system(test_join_func_future());
                // ok
                scheduler.add_system(test_join_async_and_func_future());
                // ok
                scheduler.add_system(test_when_all_func_future());
                // ok
                scheduler.add_system(test_when_all_async_future());
                // ok
                scheduler.add_system(test_channel_future_when_all());
                // ok
                scheduler.add_system(test_channel_future_join());
            }
            scheduler.update();
        } catch (std::exception e) {
            std::println("{}", e.what());
        }
        auto t = clock_record.duration_ms();
        std::println("run using {} ms", t);
        clock_record.record();
        scheduler.stop_workers();
        std::println("stop using {} s", clock_record.duration());
        std::println("app using {} s", app_record.duration());
    }
    std::println("app using {} ms", app_record.duration_ms());
    std::println("time per run {} ms", app_record.duration_ms() / run_count);
}

void test_ring_buffer() {
    RingBuffer<int, 16> buffer;
    const size_t producer_count = 3;
    const size_t consumer_count = 5;
    const size_t count_per_producer = 10000;
    const size_t expect_count = producer_count * count_per_producer;
    std::atomic_size_t comsumed = 0;

    std::vector<std::jthread> producer_threads;
    std::vector<std::jthread> consumer_threads;

    for (size_t i = 0; i < producer_count; ++i) {
        producer_threads.emplace_back([&, i]() {
            for (size_t i = 0; i < count_per_producer; ++i) {
                // std::println("try_enqueue");
                while (!buffer.try_enqueue(i))
                    // std::println("try_enqueue")
                    ;
                // std::println("producer {}", i);
                std::this_thread::sleep_for(
                    std::chrono::microseconds{std::rand() % 10});
            }
            std::println("producer {} done", i);
        });
    }
    for (size_t i = 0; i < consumer_count; ++i) {
        consumer_threads.emplace_back([&, i]() {
            int v;
            size_t c{0};
            while (expect_count != comsumed.load(std::memory_order_relaxed)) {
                using namespace std::chrono_literals;
                // std::println("try_dequeue");
                while (buffer.try_dequeue(v)) {
                    // std::println("consume {}", v);
                    comsumed.fetch_add(1, std::memory_order_relaxed);
                    ++c;
                    std::this_thread::sleep_for(
                        std::chrono::microseconds{std::rand() % 10});
                }
            }
            std::println("consumer {} done with {} object", i, c);
        });
    }
    for (auto& t : producer_threads) {
        t.join();
    }
    std::println("------------producer all done-----------");
    for (auto& t : consumer_threads) {
        t.join();
    }
    std::println("comsumed = {}", comsumed.load(std::memory_order_relaxed));
    std::println("expect_count = {}", expect_count);
}

int main(int argc, char* argv[]) {
    // test_ring_buffer();
    run_system();
    return 0;
}
