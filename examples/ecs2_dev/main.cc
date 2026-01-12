#include <float.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <print>
#include <thread>
#include <type_traits>

#include "ecs2/scheduler.hpp"
#include "ecs2/utility.hpp"
#include "ecs2/worker.hpp"
#include "ecs2/world.hpp"

namespace xc::ecs {

template <typename Fn, typename... Args>
struct Sync {
    using invoke_result_t = std::invoke_result_t<Fn, Args...>;
    Sync(Fn&& fn, Args&&... args)
        : fn_(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...)) {}

    std::function<invoke_result_t(void)> fn_;
};
template <typename Fn, typename... Args>
struct SyncWait {
    using invoke_result_t = std::invoke_result_t<Fn, Args...>;
    SyncWait(Sync<Fn, Args...>&& sync, SystemScheduler* scheduler);
    bool await_ready() { return true; }
    void await_suspend(std::coroutine_handle<SystemPromise> handle) {}
    invoke_result_t await_resume() {
        std::println("await_resume {}", (void*)this);
        uint32_t wait_count = 0;
        while (sync_flag_->test_and_set()) {
            if (wait_count++ > 1000) {
                sync_flag_->wait(true);
            }
        }
        if constexpr (std::is_void_v<invoke_result_t>) {
            sync_.fn_();
            sync_flag_->clear();
            sync_flag_->notify_one();
        } else {
            auto res = sync_.fn_();
            sync_flag_->clear();
            sync_flag_->notify_one();
            return res;
        }
    }
    std::atomic_flag* sync_flag_{nullptr};
    Sync<Fn, Args...> sync_;
};
template <typename Fn, typename... Args>
SyncWait<Fn, Args...>::SyncWait(Sync<Fn, Args...>&& sync,
                                SystemScheduler* scheduler)
    : sync_(std::move(sync)), sync_flag_(&scheduler->sync_flag_) {}

template <typename T, typename Fn>
struct AsyncForeach {
    AsyncForeach(T begin, T end, Fn&& fn) : begin(begin), end(end), fn(fn) {}
    struct AsyncForeachWait final {
        AsyncForeachWait(AsyncForeach<T, Fn>&& foreach,
                         SystemScheduler* scheduler)
            : foreach_(std::move(foreach)) {
            auto count = foreach_.end - foreach_.begin;
            auto worker_count = scheduler->worker_count();
            auto mod = count % worker_count;
            auto count_per_worker =
                (count + worker_count - 1) / worker_count - (mod != 0);
            remaning_task_count_.store(worker_count + 1);

            for (uint32_t i = 0; i < worker_count; ++i) {
                auto a = foreach_.begin + i * count_per_worker;
                auto b = (i == worker_count - 1) ? foreach_.end
                                                 : a + count_per_worker;
                auto task = [it = a, end = b, this]() mutable {
                    for (; it < end; ++it) {
                        smart_invoke(foreach_.fn, it, it - foreach_.begin);
                    }
                    if (remaning_task_count_.fetch_sub(1) == 1) {
                        if (caller_) {
                            _SCHEDULER_DEBUG("AsyncForeachWait await_resume");
                            caller_.resume();
                        }
                    }
                };
                scheduler->submit_task(task);
            }
        }

        bool await_ready() {
            auto ready = remaning_task_count_.load() == 1;
            return ready;
        }
        void await_suspend(std::coroutine_handle<SystemPromise> caller) {
            caller_ = caller;
            if (remaning_task_count_.fetch_sub(1) == 1) {
                caller.resume();
            }
        }
        void await_resume() {}

       private:
        template <typename _Fn, typename _T, typename _Offset>
        static void smart_invoke(_Fn&& fn, _T&& it, _Offset&& offset) {
            if constexpr (std::is_invocable_v<_Fn, _T, _Offset>) {
                fn(it, offset);
            } else if constexpr (std::is_invocable_v<_Fn, _T>) {
                fn(it);
            } else {
                using Vt = decltype(*it);
                if constexpr (std::is_invocable_v<_Fn, Vt>) {
                    fn(*it);
                } else if constexpr (std::is_invocable_v<_Fn, Vt, _Offset>) {
                    fn(*it, offset);
                } else {
                    static_assert(false, "invalid call");
                }
            }
        }

        AsyncForeach<T, Fn> foreach_{};
        system_handle_t caller_{nullptr};
        std::atomic_uint32_t remaning_task_count_{0};
    };
    static AsyncForeachWait await_transform(AsyncForeach<T, Fn>&& foreach,
                                            SystemScheduler* scheduler,
                                            system_handle_t handle) {
        return AsyncForeachWait(std::move(foreach), scheduler);
    }

   private:
    T begin;
    T end;
    Fn fn;
};

struct Yield final {
    struct YieldWaitable {
        bool await_ready() const noexcept { return false; }
        void await_suspend(system_handle_t handle_) const noexcept {
            handle_.promise().scheduler_->submit_handle(handle_);
        }
        void await_resume() const noexcept {}
    };
    static YieldWaitable yield(Yield, SystemScheduler* scheduler,
                               system_handle_t handle) {
        return YieldWaitable{};
    }
};

}  // namespace xc::ecs

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
