#include <float.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <functional>
#include <optional>
#include <print>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "ecs2/async_primitives.hpp"
#include "ecs2/scheduler.hpp"
#include "ecs2/types.hpp"
#include "ecs2/utility.hpp"
#include "ecs2/world.hpp"
using namespace xc::ecs;

template <typename From>
struct TaskResumWaitable {
    using from_type = From;
    using wait_type = From::wait_type;
    TaskResumWaitable() {}
    bool await_ready() { return false; }
    void await_suspend(system_handle_t caller) {
        caller.promise().scheduler_->submit_task([this, caller]() {
            if constexpr (std::is_invocable_v<decltype(&wait_type::task),
                                              wait_type*>) {
                static_cast<wait_type*>(this)->task();
            } else if constexpr (std::is_invocable_v<decltype(&wait_type::task),
                                                     wait_type*,
                                                     decltype(caller)>) {
                static_cast<wait_type*>(this)->task(caller);
            } else {
                static_assert(false,
                              "wait_type::task 必须是 void(wait_type*) 或 "
                              "void(wait_type*, system_handle_t)");
            }
            caller.resume();
        });
    }
    decltype(auto) await_resume() {
        return static_cast<wait_type*>(this)->get_result();
    }
    static wait_type await_transform(From&& future, SystemScheduler* scheduler,
                                     system_handle_t handle) {
        if constexpr (std::is_constructible_v<wait_type, From, SystemScheduler*,
                                              system_handle_t>) {
            return wait_type{std::forward<From>(future), scheduler, handle};
        } else if constexpr (std::is_constructible_v<wait_type, From>) {
            return wait_type{std::forward<From>(future)};
        } else {
            static_assert(sizeof(From) == 0,
                          "无法构造 Waiter，请检查构造函数参数");
        }
    }
};

template <typename T>
struct Future final {
    using function_t = std::function<T(void)>;

   public:
    template <typename Fn, typename Rtp = std::invoke_result_t<Fn>>
    Future(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

   public:
    struct wait_type;
    using wait_base_t = TaskResumWaitable<Future>;
    struct wait_type : wait_base_t {
        wait_type(Future&& future)
            : future_(std::move(future)), wait_base_t() {}
        void task() noexcept {
            try {
                future_.value_ = std::move(future_.fn_());
            } catch (...) {
                future_.exception_ = std::current_exception();
            }
        }
        Future&& get_result() { return std::move(future_); }
        Future future_;
    };
    static constexpr auto await_transform =
        TaskResumWaitable<Future>::await_transform;

   public:
    T& value() {
        if (exception_) std::rethrow_exception(exception_);
        return value_.value();
    }

   public:
    Future(Future&&) = default;
    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;
    Future& operator=(Future&&) = delete;

   private:
    std::optional<T> value_{std::nullopt};
    std::exception_ptr exception_{nullptr};
    function_t fn_{};
};
template <typename Fn, typename Rtp = std::invoke_result_t<Fn>>
Future(Fn&&) -> Future<Rtp>;

template <typename... T>
struct Join {
    struct wait_type;
    using wait_base_t = TaskResumWaitable<Join>;
    struct wait_type : wait_base_t {
        wait_type(Join&& join) : join_(std::move(join)), wait_base_t() {}
        void task(system_handle_t caller) noexcept {
            [this, caller]<size_t... I>(std::index_sequence<I...>) {
                (caller.promise().scheduler_->submit_task([this, caller]() {
                    using Otp =
                        std::decay_t<decltype(std::get<I>(join_.tasks_))>;
                    Otp::await_transform(std::move(std::get<I>(join_.tasks_)),
                                         caller.promise().scheduler_, caller);
                    if (remaning_task_count_.fetch_sub(1) == 1) {
                        caller.resume();
                    }
                }),
                 ...);
            }(std::make_index_sequence<sizeof...(T)>());
        }
        Join&& get_result() { return std::move(join_); }
        Join join_;
        std::atomic_uint32_t remaning_task_count_{0};
    };
    static constexpr auto await_transform =
        TaskResumWaitable<Join>::await_transform;
    decltype(auto) values() {
        return [this]<size_t... I>(std::index_sequence<I...>) {
            return std::make_tuple(std::get<I>(tasks_).get_result()...);
        }(std::make_index_sequence<sizeof...(T)>());
    }

    Join(T&&... tasks) : tasks_(std::forward_as_tuple(tasks...)) {}
    std::tuple<T...> tasks_{};
};
template <typename... T>
Join(T&&...) -> Join<T...>;

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
    // auto t1 = AsyncForeach{0, 100, [](int i) { std::println("i={}", i); }};
    // auto t2 = AsyncForeach{0, 100, [](int i) { std::println("i={}", i + 1);
    // }};

    // co_await Join(t1, t2);
    auto f1 = co_await Future{[]() {
        for (size_t i = 0; i < 100; ++i) {
            std::println("i={}", i);
        }
        return 11;
    }};
    auto f2 = co_await Future{[]() {
        for (size_t j = 0; j < 100; ++j) {
            std::println("j={}", j);
        }
        return 11;
    }};
    co_await Join{f1, f2};

    // std::println("f {}", f.value());
    auto tasks = Join{1, 2, 3};
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
    return 0;
}
